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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include <iostream>
#include <iomanip>

#include "pefile.h"
#include "pemap.h"
#include "pepatch.h"

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

    friend std::ostream& operator<<(std::ostream& os, const Patch& patch);

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
};

std::ostream& operator<<(std::ostream& os, const Patch& patch) {
    os << "Patching '" << patch.name
       << "' at offset 0x" << std::hex << patch.offset << std::dec
       << " (" << patch.length << " bytes)";
    return os;
}

class Patches
{
private:
    // List of patches
    std::vector<Patch> _patches;

    uint8_t* _buf;

public:

    Patches(uint8_t* buf) : _buf(buf) {}

    void add(Patch patch) {
        _patches.push_back(patch);
    }

    template<typename T>
    void add(const T* addr, const T* data, const char* name = NULL)
    {
        add(Patch((const uint8_t*)addr - _buf, data, name));
    }

    void apply(bool dryRun = false) {
        for (auto&& patch: _patches)
            patch.apply(_buf, dryRun);
    }
};

/**
 * Helper class to parse image headers.
 */
class PEFile
{
private:

    const uint8_t* _buf;
    const size_t _length;

    // Pointer to the optional header.
    const uint8_t* _optional;

    void _init();

public:

    const IMAGE_DOS_HEADER* dosHeader;
    const IMAGE_FILE_HEADER* fileHeader;
    const IMAGE_SECTION_HEADER* sectionHeaders;


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
        if (_optional + sizeof(T) >= _buf + _length)
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

        return _buf + rva - s->VirtualAddress + s->PointerToRawData;
    }

    /**
     * Checks if the given pointer for type T is contained in the image. That
     * is, if the object it points to fits inside as well.
     */
    template<typename T>
    bool isValidReference(const T* p) const {
        return ((const uint8_t*)p >= _buf) &&
               ((const uint8_t*)p + sizeof(T) <= _buf + _length);
    }
};

PEFile::PEFile(const uint8_t* buf, size_t length)
    : _buf(buf), _length(length)
{
    _init();
}

void PEFile::_init() {

    const uint8_t* p = _buf;
    const uint8_t* end = _buf + _length;

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
        uint32_t entry, const char* name, uint32_t timestamp) {

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
        patches.add(&dir->TimeDateStamp, &timestamp, name);
}

/**
 * There are 0 or more debug data directories. We need to patch the timestamp in
 * all of them.
 */
void patchDebugDataDirectories(const PEFile& pe, Patches& patches,
        const IMAGE_DATA_DIRECTORY* imageDataDirs, uint32_t timestamp) {

    const IMAGE_DATA_DIRECTORY& imageDataDir = imageDataDirs[IMAGE_DIRECTORY_ENTRY_DEBUG];

    // Doesn't exist? Nothing to patch.
    if (imageDataDir.VirtualAddress == 0)
        return;

    const uint8_t* p = pe.translate(imageDataDir.VirtualAddress);
    if (!pe.isValidReference(p))
        throw InvalidImage("invalid IMAGE_DATA_DIRECTORY.VirtualAddress is invalid");

    // The first debug data directory
    const IMAGE_DEBUG_DIRECTORY* dir = (const IMAGE_DEBUG_DIRECTORY*)p;

    // There can be multiple debug data directories in this section. This is how
    // to calculate the number of them.
    const size_t debugDirCount = imageDataDir.Size /
        sizeof(IMAGE_DEBUG_DIRECTORY);

    for (size_t i = 0; i < debugDirCount; ++i) {

        if (!pe.isValidReference(dir))
            throw InvalidImage("IMAGE_DEBUG_DIRECTORY is not valid");

        if (dir->TimeDateStamp != 0)
            patches.add(&dir->TimeDateStamp, &timestamp,
                    "IMAGE_DEBUG_DIRECTORY.TimeDateStamp");

        ++dir;
    }
}

/**
 * Patches the image based on the optional header type. The optional header can
 * be either 32- or 64-bit.
 */
template<typename T>
void patchOptionalHeader(const PEFile& pe,
        Patches& patches, uint32_t timestamp) {

    const T* optional = pe.optional<T>();

    patches.add(&optional->CheckSum, &timestamp,
            "OptionalHeader.CheckSum");

    const IMAGE_DATA_DIRECTORY* dataDirs = optional->DataDirectory;

    // Patch exports directory timestamp
    patchDataDirectory<IMAGE_EXPORT_DIRECTORY>(pe, patches,
            dataDirs, IMAGE_DIRECTORY_ENTRY_EXPORT,
            "IMAGE_EXPORT_DIRECTORY.TimeDateStamp", timestamp);

    // Patch resource directory timestamp
    patchDataDirectory<IMAGE_RESOURCE_DIRECTORY>(pe, patches,
            dataDirs, IMAGE_DIRECTORY_ENTRY_RESOURCE,
            "IMAGE_RESOURCE_DIRECTORY.TimeDateStamp", timestamp);

    // Patch the debug directories
    patchDebugDataDirectories(pe, patches, dataDirs, timestamp);
}

}

void patchImage(const char* imagePath, const char* pdbPath, bool dryRun) {
    MemMap image(imagePath);
    MemMap pdb(pdbPath);

    // Replacement for timestamps
    const uint32_t timestamp = 0;

    uint8_t* buf = (uint8_t*)image.buf();
    const size_t length = image.length();

    PEFile pe = PEFile(buf, length);

    Patches patches(buf);

    patches.add(&pe.fileHeader->TimeDateStamp, &timestamp,
            "IMAGE_FILE_HEADER.TimeDateStamp");

    switch (pe.magic()) {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
            // Patch as a PE32 file
            patchOptionalHeader<IMAGE_OPTIONAL_HEADER32>(pe, patches,
                    timestamp);
            break;

        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
            // Patch as a PE32+ file
            patchOptionalHeader<IMAGE_OPTIONAL_HEADER64>(pe, patches,
                    timestamp);
            break;

        default:
            throw InvalidImage("unsupported IMAGE_NT_HEADERS.OptionalHeader");
    }

    patches.apply(dryRun);
}
