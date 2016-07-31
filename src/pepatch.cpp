/*
 * Copyright (c) 2016 Jason White
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * This file contains the core logic for parsing the PE file, finding the places
 * in the PE file that need to be overwritten, and overwriting those places.
 *
 * In general, it works like this:
 *
 *  1. Map the PE file into memory. Since we are not changing the size of the
 *     file, this is much more efficient than simply loading the whole file into
 *     memory the naive way. To write to the file, we simply set values in the
 *     appropriate memory locations.
 *
 *  2. Parse the PE headers. Until we get to the optional header, there are no
 *     differences between the PE and PE+ formats (32- and 64-bit images). We
 *     must parse the optional header differently depending on which format the
 *     file is.
 *
 *  3. After the main headers are parsed, we start marking regions in the file
 *     that need to be patched. Note that we do not overwrite these locations
 *     immediately because there is still more parsing to do. If the parsing
 *     fails, we do not want to end up in an inconsistent state. The potential
 *     for a partial success/failure state should be minimized as much as
 *     possible. Thus, we do not apply the patches until the very end. The main
 *     places to patch include:
 *
 *     a. Time stamps occur in few places in the main headers. We patch all of
 *        these with a semi-arbitrary timestamp of Jan 1, 2010, 0:00:00 GMT. We
 *        cannot use 0 as that has a special meaning. Rather than being
 *        inconsistent with prior work, we use the same one as Google's
 *        zap_timestamp utility.
 *     b. Next, we patch the data directories. There are three of them with
 *        non-reproducible data: IMAGE_EXPORT_DIRECTORY,
 *        IMAGE_RESOURCE_DIRECTORY, and IMAGE_DEBUG_DIRECTORY. The tricky one is
 *        the debug directory. This includes a signature to match the PE file
 *        with the PDB file. We patch this with an MD5 checksum of the PE file,
 *        skipping over the patched areas. This checksum is calculated after
 *        all of the patches are added. When the patches are applied, this is
 *        what will be set.
 *
 *  4. Finally, the patches are applied.
 *
 * References:
 * - https://msdn.microsoft.com/en-us/library/ms809762.aspx
 * - http://www.debuginfo.com/articles/debuginfomatch.html
 * - https://github.com/google/syzygy/
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <vector>
#include <iostream>
#include <iomanip>
#include <tuple>

#include "pefile.h"
#include "pemap.h"
#include "pepatch.h"
#include "md5.h"

namespace {

/**
 * A range of memory to patch. This is used to keep track of what needs to be
 * patched in the PE file.
 *
 * All the patch locations need to be found before finishing parsing. If we
 * patched while parsing, then parsing could fail and we could be left with an
 * incomplete patch. Thus, we keep a list of patches and patch everything all at
 * once to mitigate failure cases.
 */
struct Patch
{
    // Location to patch.
    size_t offset;

    // Length of the data.
    size_t length;

    // Data overwrite the given location with.
    const uint8_t* data;

    // Name of the patch. Useful to see what's going on.
    const char* name;

    Patch(size_t offset, size_t length, const uint8_t* data, const char* name = NULL)
        : offset(offset),
          length(length),
          data(data),
          name(name)
    {}

    template<typename T>
    Patch(size_t offset, const T* data, const char* name = NULL)
        : offset(offset),
          length(sizeof(T)),
          data((const uint8_t*)data),
          name(name)
    {
    }

    /**
     * Applies the patch. Note that no bounds checking is done. It is assumed
     * that it has already been done.
     */
    void apply(uint8_t* buf, bool dryRun) {
        std::cout << *this << std::endl;

        if (!dryRun) {
            for (size_t i = 0; i < length; ++i)
                buf[offset+i] = data[i];
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const Patch& patch) {
        os << "Patching '" << patch.name
           << "' at offset 0x" << std::hex << patch.offset << std::dec
           << " (" << patch.length << " bytes)";
        return os;
    }

    // Implement ordering for sorting purposes.
    friend bool operator<(const Patch& a, const Patch& b) {
        return std::tie(a.offset, a.length) < std::tie(b.offset, b.length);
    }
};

class Patches
{
private:

    uint8_t* _buf;

public:
    // List of patches
    std::vector<Patch> patches;

    Patches(uint8_t* buf) : _buf(buf) {}

    void add(Patch patch) {
        patches.push_back(patch);
    }

    /**
     * Convenience function for adding patches.
     */
    template<typename T>
    void add(const T* addr, const T* data, const char* name = NULL)
    {
        add(Patch((const uint8_t*)addr - _buf, data, name));
    }

    /**
     * Sort the patches. The patches will be ordered according to the offset in
     * the file. This is useful once all the patches have been added, but not
     * applied so that we can take the checksum of the file in the areas between
     * the patches.
     */
    void sort() {
        std::sort(patches.begin(), patches.end());
    }

    void apply(bool dryRun = false) {
        for (auto&& patch: patches)
            patch.apply(_buf, dryRun);
    }
};

/**
 * Helper class to parse image headers.
 */
class PEFile
{
private:

    // Pointer to the optional header.
    const uint8_t* _optional;

    void _init();

public:

    const uint8_t* buf;
    const size_t length;

    const IMAGE_DOS_HEADER* dosHeader;
    const IMAGE_FILE_HEADER* fileHeader;
    const IMAGE_SECTION_HEADER* sectionHeaders;

    // Replacement for timestamps
    //
    // The timestamp can't just be set to zero as that represents a special
    // value in the PE file. We set it to some arbitrary fixed date in the past.
    // This is Jan 1, 2010, 0:00:00 GMT. This date shouldn't be too far in the
    // past, otherwise Windows might trigger a warning saying that the
    // instrumented image has known incompatibility issues when someone tries to
    // run it.
    const uint32_t timestamp = 1262304000;

    // Replacement for the PDB age. Starting at 1, this is normally incremented
    // every time the PDB file is incrementally updated. However, for our
    // purposes, we want to keep this at 1.
    const uint32_t pdbAge = 1;

    // Replacement for the PDB GUID. This is calculated by taking the MD5
    // checksum of the PE file skipping over the parts that we patch. Thus, we
    // can mark the PDB signature to be patched with these bytes, but calculate
    // the signature before actually applying the patch.
    uint8_t pdbSignature[16];

    PEFile(const uint8_t* buf, size_t length);

    /**
     * The Magic field of the optional header. This is used to determine if the
     * optional header is 32- or 64-bit.
     */
    uint16_t magic() const {
        return *(uint16_t*)_optional;
    }

    /**
     * Returns the optional header of the given type.
     */
    template<typename T>
    const T* optional() const {
        // Bounds check
        if (_optional + sizeof(T) >= buf + length)
            throw InvalidImage("missing IMAGE_OPTIONAL_HEADER");

        return (T*)_optional;
    }

    /**
     * Translates a relative virtual address (RVA) to a physical address within
     * the mapped file. Note that this does not do any bounds checking. You must
     * check the bounds before dereferencing the returned pointer.
     */
    const uint8_t* translate(size_t rva) const {

        const IMAGE_SECTION_HEADER* s = sectionHeaders;

        for (size_t i = 0; i < fileHeader->NumberOfSections; ++i) {
            if (rva >= s->VirtualAddress &&
                rva < s->VirtualAddress + s->Misc.VirtualSize)
                break;

            ++s;
        }

        return buf + rva - s->VirtualAddress + s->PointerToRawData;
    }

    /**
     * Checks if the given pointer for type T is contained in the image. That
     * is, if the object it points to fits inside as well.
     */
    template<typename T>
    bool isValidReference(const T* p) const {
        return ((const uint8_t*)p >= buf) &&
               ((const uint8_t*)p + sizeof(T) <= buf + length);
    }
};

PEFile::PEFile(const uint8_t* buf, size_t length)
    : buf(buf), length(length)
{
    memset(pdbSignature, 0, sizeof(pdbSignature)/sizeof(*pdbSignature));

    _init();
}

void PEFile::_init() {

    const uint8_t* p = buf;
    const uint8_t* end = buf + length;

    if (p + sizeof(IMAGE_DOS_HEADER) >= end)
        throw InvalidImage("missing DOS header");

    dosHeader = (IMAGE_DOS_HEADER*)p;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        throw InvalidImage("invalid DOS signature");

    // Skip to the NT headers. Note that we don't parse this section as
    // IMAGE_NT_HEADERS32/IMAGE_NT_HEADERS64 because we don't yet know if this
    // image is 32- or 64-bit. That information is in the first field of the
    // optional header.
    p += dosHeader->e_lfanew;

    //
    // Check the signature
    //
    if (p + sizeof(uint32_t) >= end)
        throw InvalidImage("missing PE signature");

    const uint32_t signature = *(uint32_t*)p;
    if (signature != *(const uint32_t*)"PE\0\0")
        throw InvalidImage("invalid PE signature");

    p += sizeof(signature);

    //
    // Parse the image file header
    //
    if (p + sizeof(IMAGE_FILE_HEADER) >= end)
        throw InvalidImage("missing IMAGE_FILE_HEADER");

    fileHeader = (IMAGE_FILE_HEADER*)p;

    p += sizeof(IMAGE_FILE_HEADER);

    //
    // The optional header is here. Parsing of this is delayed because it can
    // either be a 32- or 64-bit structure.
    //
    _optional = p;

    p += fileHeader->SizeOfOptionalHeader;

    //
    // Section headers. There are IMAGE_FILE_HEADER.NumberOfSections of these.
    //
    sectionHeaders = (IMAGE_SECTION_HEADER*)p;
}

/**
 * Patches the timestamp associated with a data directory.
 */
template<typename T>
void patchDataDirectory(const PEFile& pe, Patches& patches,
        const IMAGE_DATA_DIRECTORY* imageDataDirs,
        uint32_t entry, const char* name) {

    const IMAGE_DATA_DIRECTORY& imageDataDir = imageDataDirs[entry];

    // Doesn't exist? Nothing to patch.
    if (imageDataDir.VirtualAddress == 0)
        return;

    if (imageDataDir.Size < sizeof(T)) {
        // Note that we check if the size is less than our defined struct.
        // Microsoft is free to add elements to the end of the struct in future
        // versions as that still maintains ABI compatibility.
        throw InvalidImage("IMAGE_DATA_DIRECTORY.Size is invalid");
    }

    const uint8_t* p = pe.translate(imageDataDir.VirtualAddress);
    if (!pe.isValidReference(p))
        throw InvalidImage("IMAGE_DATA_DIRECTORY.VirtualAddress is invalid");

    const T* dir = (const T*)p;

    if (dir->TimeDateStamp != 0)
        patches.add(&dir->TimeDateStamp, &pe.timestamp, name);
}

/**
 * There are 0 or more debug data directories. We need to patch the timestamp in
 * all of them.
 */
void patchDebugDataDirectories(const PEFile& pe, Patches& patches,
        const IMAGE_DATA_DIRECTORY* imageDataDirs) {

    const IMAGE_DATA_DIRECTORY& imageDataDir = imageDataDirs[IMAGE_DIRECTORY_ENTRY_DEBUG];

    // Doesn't exist? Nothing to patch.
    if (imageDataDir.VirtualAddress == 0)
        return;

    const uint8_t* p = pe.translate(imageDataDir.VirtualAddress);
    if (!pe.isValidReference(p))
        throw InvalidImage("invalid IMAGE_DATA_DIRECTORY.VirtualAddress is invalid");

    // The first debug data directory
    const IMAGE_DEBUG_DIRECTORY* dir = (const IMAGE_DEBUG_DIRECTORY*)p;

    // There can be multiple debug data directories in this section.
    const size_t debugDirCount = imageDataDir.Size /
        sizeof(IMAGE_DEBUG_DIRECTORY);

    // Information about the PDB.
    const CV_INFO_PDB70* cvInfo = NULL;

    // Patch all of the debug data directories. Note that, at most, one of these
    // will be of type IMAGE_DEBUG_TYPE_CODEVIEW. We will use this to also patch
    // the PDB.
    for (size_t i = 0; i < debugDirCount; ++i) {

        if (!pe.isValidReference(dir))
            throw InvalidImage("IMAGE_DEBUG_DIRECTORY is not valid");

        if (dir->TimeDateStamp != 0)
            patches.add(&dir->TimeDateStamp, &pe.timestamp,
                    "IMAGE_DEBUG_DIRECTORY.TimeDateStamp");

        if (dir->Type == IMAGE_DEBUG_TYPE_CODEVIEW) {

            if (cvInfo)
                throw InvalidImage("found multiple CodeView debug entries");

            cvInfo = (const CV_INFO_PDB70*)(pe.buf + dir->PointerToRawData);

            if (!pe.isValidReference(cvInfo))
                throw InvalidImage("invalid CodeView debug entry location");
        }

        ++dir;
    }

    if (cvInfo) {

        if (cvInfo->CvSignature != CV_INFO_SIGNATURE_PDB70)
            throw InvalidImage("unsupported PDB format, only version 7.0 is supported");

        patches.add(&cvInfo->Signature, &pe.pdbSignature, "PDB Signature");
        patches.add(&cvInfo->Age, &pe.pdbAge, "PDB Age");
    }
}

/**
 * Patches the image based on the optional header type. The optional header can
 * be either 32- or 64-bit.
 */
template<typename T>
void patchOptionalHeader(const PEFile& pe, Patches& patches) {

    const T* optional = pe.optional<T>();

    patches.add(&optional->CheckSum, &pe.timestamp,
            "OptionalHeader.CheckSum");

    const IMAGE_DATA_DIRECTORY* dataDirs = optional->DataDirectory;

    // Patch exports directory timestamp
    patchDataDirectory<IMAGE_EXPORT_DIRECTORY>(pe, patches,
            dataDirs, IMAGE_DIRECTORY_ENTRY_EXPORT,
            "IMAGE_EXPORT_DIRECTORY.TimeDateStamp");

    // Patch resource directory timestamp
    patchDataDirectory<IMAGE_RESOURCE_DIRECTORY>(pe, patches,
            dataDirs, IMAGE_DIRECTORY_ENTRY_RESOURCE,
            "IMAGE_RESOURCE_DIRECTORY.TimeDateStamp");

    // Patch the debug directories
    patchDebugDataDirectories(pe, patches, dataDirs);
}

/**
 * Calculates the checksum for the PE image, skipping over patched areas. This
 * is used as the PDB signature.
 *
 * The list of patches is assumed to be sorted.
 *
 * Note that this uses the MD5 hashing algorithm, but any 128-bit hashing
 * algorithm could be used instead. It might be a good idea to use a hashing
 * algorithm with better distribution to avoid collisions. MurmurHash3 could be
 * a good choice, but it can't incrementally hash chunks of data.
 */
void calculateChecksum(const uint8_t* buf, const size_t length,
        const std::vector<Patch>& patches, uint8_t output[16]) {

    size_t pos = 0;

    md5_context ctx;
    md5_starts(&ctx);

    // Take the checksum of the regions between the patches to ensure a
    // deterministic file checksum. Since the patches are sorted, we iterate
    // over the file sequentially.
    for (auto&& patch: patches) {
        // Hash everything up to the patch
        md5_update(&ctx, buf + pos, patch.offset - pos);

        // Skip past the patch
        pos = patch.offset + patch.length;
    }

    // Get everything after the last patch
    md5_update(&ctx, buf + pos, length - pos);

    md5_finish(&ctx, output);
}

template<typename CharT>
void patchImageImpl(const CharT* imagePath, const CharT* pdbPath, bool dryRun) {
    MemMap image(imagePath);

    uint8_t* buf = (uint8_t*)image.buf();
    const size_t length = image.length();

    PEFile pe = PEFile(buf, length);

    Patches patches(buf);

    patches.add(&pe.fileHeader->TimeDateStamp, &pe.timestamp,
            "IMAGE_FILE_HEADER.TimeDateStamp");

    switch (pe.magic()) {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
            // Patch as a PE32 file
            patchOptionalHeader<IMAGE_OPTIONAL_HEADER32>(pe, patches);
            break;

        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
            // Patch as a PE32+ file
            patchOptionalHeader<IMAGE_OPTIONAL_HEADER64>(pe, patches);
            break;

        default:
            throw InvalidImage("unsupported IMAGE_NT_HEADERS.OptionalHeader");
    }

    patches.sort();

    // Calculate the checksum of the PE file. Note that the checksum is stored
    // in PDB signature. When the patches are applied, this checksum is what
    // will be set in the file.
    calculateChecksum(buf, length, patches.patches, pe.pdbSignature);

    if (pdbPath) {
        MemMap pdb(pdbPath);
    }

    patches.apply(dryRun);
}

}

void patchImage(const char* imagePath, const char* pdbPath, bool dryrun) {
    patchImageImpl(imagePath, pdbPath, dryrun);
}

#ifdef _WIN32
void patchImage(const wchar_t* imagePath, const wchar_t* pdbPath, bool dryrun) {
    patchImageImpl(imagePath, pdbPath, dryrun);
}
#endif
