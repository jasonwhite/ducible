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
 *     a. Timestamps that occur in the main headers. We patch all of these with
 *        a semi-arbitrary timestamp of Jan 1, 2010, 0:00:00 GMT. We cannot use
 *        0 as that has a special meaning. Rather than being inconsistent with
 *        prior work, we use the same one as Google's zap_timestamp utility.
 *     b. Timestamps in the data directories. There are three of them with
 *        non-reproducible data: IMAGE_EXPORT_DIRECTORY,
 *        IMAGE_RESOURCE_DIRECTORY, and IMAGE_DEBUG_DIRECTORY. The tricky one is
 *        the debug directory. In addition to a timestamp, this includes a
 *        signature to match the PE file with the PDB file. We patch this with
 *        an MD5 checksum of the PE file, skipping over the patched areas. This
 *        checksum is calculated after all of the patches are added. When the
 *        patches are applied, this is what will be set.
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

#include <cstring>
#include <iostream>
#include <vector>
#include <regex>

#include "patch_image.h"

#include "patches.h"
#include "pe_file.h"
#include "file.h"

#include "msf.h"
#include "msf_stream.h"
#include "msf_memory_stream.h"
#include "pdb.h"

#include "memmap.h"
#include "md5.h"

namespace {

/**
 * There are 0 or more debug data directories. We need to patch the timestamp in
 * all of them.
 */
template<typename OptHeader>
void patchDebugDataDirectories(const PEFile& pe, Patches& patches,
        const OptHeader* opt) {

    size_t debugDirCount;
    auto dir = pe.getDebugDataDirs(opt, debugDirCount);

    // Information about the PDB.
    const CV_INFO_PDB70* cvInfo = NULL;

    // Patch all of the debug data directories. Note that, at most, one of these
    // will be of type IMAGE_DEBUG_TYPE_CODEVIEW. We will use this to also patch
    // the PDB.
    for (size_t i = 0; i < debugDirCount; ++i) {
        if (dir->TimeDateStamp != 0)
            patches.add(&dir->TimeDateStamp, &pe.timestamp,
                    "IMAGE_DEBUG_DIRECTORY.TimeDateStamp");

        if (dir->Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
            if (cvInfo)
                throw InvalidImage("found multiple CodeView debug entries");

            cvInfo = (const CV_INFO_PDB70*)(pe.buf + dir->PointerToRawData);

            if (!pe.isValidRef(cvInfo))
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
void patchOptionalHeader(const PEFile& pe, Patches& patches, const T* optional) {

    patches.add(&optional->CheckSum, &pe.timestamp,
            "OptionalHeader.CheckSum");

    // Patch exports directory timestamp
    if (auto dir = pe.getDataDir<IMAGE_EXPORT_DIRECTORY>(optional,
                IMAGE_DIRECTORY_ENTRY_EXPORT)) {
        patches.add(&dir->TimeDateStamp, &pe.timestamp,
                "IMAGE_EXPORT_DIRECTORY.TimeDateStamp");
    }

    // Patch resource directory timestamp
    if (auto dir = pe.getDataDir<IMAGE_RESOURCE_DIRECTORY>(optional,
                IMAGE_DIRECTORY_ENTRY_RESOURCE)) {
        patches.add(&dir->TimeDateStamp, &pe.timestamp,
                "IMAGE_RESOURCE_DIRECTORY.TimeDateStamp");
    }

    // Patch the debug directories
    patchDebugDataDirectories(pe, patches, optional);
}

/**
 * Calculates the checksum for the PE image, skipping over patched areas. This
 * is used to replace the PDB signature with something that is deterministic.
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

/**
 * Compares the PE and PDB signatures to see if they match.
 */
bool matchingSignatures(const CV_INFO_PDB70& pdbInfo,
                        const PdbStream70& pdbHeader) {
    if (pdbInfo.Age != pdbHeader.age ||
        memcmp(pdbInfo.Signature, pdbHeader.sig70, sizeof(pdbHeader.sig70)) != 0
        ) {
        return false;
    }

    return true;
}

template<typename CharT>
constexpr CharT tmpSuffix[] = {};

template<> constexpr char tmpSuffix<char>[] = ".tmp";
template<> constexpr wchar_t tmpSuffix<wchar_t>[] = L".tmp";

/**
 * Returns a temporary PDB path name. The PDB will be written here first and
 * then renamed to the original after everything succeeds.
 */
template<typename CharT>
std::basic_string<CharT> getTempPdbPath(const CharT* pdbPath) {
    std::basic_string<CharT> temp(pdbPath);
    temp.append(tmpSuffix<CharT>);
    return temp;
}

template<typename CharT>
constexpr CharT nullGuid[] = {};

template<> constexpr char nullGuid<char>[] = "{00000000-0000-0000-0000-000000000000}";
template<> constexpr wchar_t nullGuid<wchar_t>[] = L"{00000000-0000-0000-0000-000000000000}";

/**
 * Helper function for normalizing a GUID in a NULL terminated file name.
 */
template <typename CharT>
void normalizeFileNameGuid(CharT* path, size_t length) {
    static const std::regex guidRegex("\\{"
            "[0-9a-fA-F]{8}-"
            "[0-9a-fA-F]{4}-"
            "[0-9a-fA-F]{4}-"
            "[0-9a-fA-F]{4}-"
            "[0-9a-fA-F]{12}"
        "\\}");

    std::match_results<const CharT*> match;

    if (std::regex_search((const CharT*)path, (const CharT*)path + length,
                match, guidRegex)) {
        memcpy(path + match.position(0), nullGuid<CharT>,
                sizeof(nullGuid<CharT>));
    }
}

/**
 * Patches the DBI stream.
 */
void patchDbiStream(MsfMemoryStream* stream) {

    if (stream->length() < sizeof(DbiHeader))
        throw InvalidPdb("DBI stream too short");

    uint8_t* data = stream->data();
    const size_t length = stream->length();
    size_t offset = 0;

    DbiHeader* dbiHeader = (DbiHeader*)data;

    // Sanity checks
    if (dbiHeader->signature != dbiHeaderSignature)
        throw InvalidPdb("invalid DBI header signature");

    if (dbiHeader->version != DbiVersion::v70)
        throw InvalidPdb("Unsupported DBI stream version");

    // Patch the age. This must match the age in the PDB stream.
    dbiHeader->age = 1;

    offset += sizeof(*dbiHeader);

    // The module info immediately follows the header.

    // Check bounds
    if (offset + dbiHeader->gpModInfoSize > length)
        throw InvalidPdb("DBI module info size exceeds stream length");

    // Number of modules
    size_t moduleCount = 0;

    // Patch the module info entries
    for (size_t i = 0; i < dbiHeader->gpModInfoSize; ) {
        if (dbiHeader->gpModInfoSize - i < sizeof(ModuleInfo))
            throw InvalidPdb("got partial DBI module info");

        ModuleInfo* info = (ModuleInfo*)(data + offset + i);

        // Patch the offsets "array". This is not used directly by Microsoft's
        // DBI implementation and may contain non-deterministic data (e.g., the
        // memory address of the actual allocated array). Thus, we need to zero
        // it out.
        info->offsets = 0;

        i += info->size();
        ++moduleCount;
    }

    offset += dbiHeader->gpModInfoSize;

    // The section contributions follow the module info entries. These contain
    // garbage due to struct alignment. They needed to be zeroed out.

    if (offset + dbiHeader->sectionContributionSize > length) {
        throw InvalidPdb(
                "DBI section contributions size exceeds stream length");
    }

    const size_t scCount = dbiHeader->sectionContributionSize /
        sizeof(SectionContribution);

    SectionContribution* sectionContribs = (SectionContribution*)(data + offset);

    for (size_t i = 0; i < scCount; ++i) {
        SectionContribution& sc = sectionContribs[i];
        sc.padding1 = 0;
        sc.padding2 = 0;
    }

    offset += dbiHeader->sectionContributionSize;

    // Skip over the section map
    offset += dbiHeader->sectionMapSize;

    // In the list of files, there are some temporary files with random GUIDs in
    // the name.
    if (dbiHeader->fileInfoSize > 0) {

        if (offset + dbiHeader->fileInfoSize > length)
            throw InvalidPdb("Missing file info in DBI stream");

        uint8_t* p = data + offset;
        uint8_t* pEnd = p + dbiHeader->fileInfoSize;

        // Skip over the header as it doesn't always provide correct
        // information.
        p += sizeof(FileInfoHeader);

        // File indices array
        uint16_t* fileIndices = (uint16_t*)p;
        p += moduleCount * sizeof(*fileIndices);

        // File counts array
        uint16_t* fileCounts = (uint16_t*)p;
        p += moduleCount * sizeof(*fileCounts);

        if (p >= pEnd)
            throw InvalidPdb("got partial file info in DBI stream");

        uint32_t* offsets = (uint32_t*)p;

        uint32_t offsetCount = 0;
        for (size_t i = 0; i < moduleCount; ++i)
            offsetCount += fileCounts[i];

        p += offsetCount * sizeof(*offsets);

        if (p >= pEnd)
            throw InvalidPdb("got partial file info in DBI stream");

        char* names = (char*)p;

        for (size_t i = 0; i < offsetCount; ++i) {
            uint32_t offset = offsets[i];

            if ((uint8_t*)names + offset + 1 > pEnd)
                throw InvalidPdb("invalid offset for file info name");

            char* name = names + offset;
            size_t len = strlen(name);

            if ((uint8_t*)name + len + 1 > pEnd)
                throw InvalidPdb("file name exceeds file info section size");

            normalizeFileNameGuid(name, len);
        }
    }
}

/**
 * Patches the symbol record stream.
 *
 * There is up to 3 bytes of padding at the end of each symbol record. Since
 * garbage just lives there, it needs to be zeroed out.
 */
void patchSymbolRecordsStream(MsfMemoryStream* stream) {

    uint8_t* data = stream->data();
    const size_t length = stream->length();

    for (size_t i = 0; i < length; ) {

        if (length - i < sizeof(SymbolRecord))
            throw InvalidPdb("got partial symbol record");

        SymbolRecord* rec = (SymbolRecord*)(data + i);

        // The symbol record length must be at least the size of
        // SymbolRecord::type and the size of the entire record must be a
        // multiple of 4.
        if (rec->length < sizeof(rec->type) ||
            (rec->length + sizeof(rec->length)) % 4 != 0) {
            throw InvalidPdb("invalid symbol record size");
        }

        const size_t dataLength = rec->length - sizeof(rec->type);

        // Bounds check.
        if (i + sizeof(SymbolRecord) + dataLength > length)
            throw InvalidPdb("symbol record size too large");

        // There is a maximum of 3 bytes of padding at the end of the data.
        // Note that if the data length is < 3 and this overflows,
        size_t tail = dataLength - 3;

        // Find the null terminator at the end. The padding (if any) will be
        // after this point.
        while (tail + 1 < dataLength && rec->data[tail] != 0)
            ++tail;

        // Zero out the padding.
        while (tail < dataLength)
            rec->data[tail++] = 0;

        // Skip to next symbol record
        i += sizeof(SymbolRecord) + dataLength;
    }
}

/**
 * Patch the public symbol info stream.
 */
void patchPublicSymbolStream(MsfMemoryStream* stream) {

    // The public symbol info stream starts with the public symbol header
    // followed by the (Global Symbol Info) GSI hash header. We only care about
    // the public symbol header.
    if (stream->length() < sizeof(PublicSymbolHeader))
        throw InvalidPdb("public symbol stream too short");

    PublicSymbolHeader* header = (PublicSymbolHeader*)stream->data();

    // Struct alignment padding
    header->padding1 = 0;

    // Microsoft's PDB writer has a bug where this field is not initialized in
    // the constructor. However, there are other code paths that do initialize
    // this value, but only sometimes. Thus, since Microsoft's tools are already
    // broken because of this, we zero this out without worrying about it.
    //
    // Since fixing this would be a trivial one-liner for Microsoft, this patch
    // could become silently obsolete in the future.
    header->sectionCount = 0;
}

/**
 * Rewrites a PDB, eliminating non-determinism.
 */
void patchPDB(MsfFile& msf, const CV_INFO_PDB70* pdbInfo,
        uint32_t timestamp, const uint8_t signature[16]) {

    msf.replaceStream((size_t)PdbStreamType::streamTable, nullptr);

    // Read the PDB header
    auto pdbHeaderStream = msf.getStream((size_t)PdbStreamType::header);

    if (!pdbHeaderStream)
        throw InvalidPdb("missing PDB header stream");

    if (pdbHeaderStream->length() < sizeof(PdbStream70))
        throw InvalidPdb("missing PDB 7.0 header");

    PdbStream70 pdbHeader;
    if (pdbHeaderStream->read(sizeof(pdbHeader), &pdbHeader) !=
            sizeof(pdbHeader))
        throw InvalidPdb("missing PDB header");

    if (pdbHeader.version < PdbVersion::vc70)
        throw InvalidPdb("unsupported PDB implementation version");

    // Check that this PDB matches what the PE file expects
    if (!pdbInfo || !matchingSignatures(*pdbInfo, pdbHeader))
        throw InvalidPdb("PE and PDB signatures do not match");

    // Patch the PDB header stream
    pdbHeader.timestamp = timestamp;
    pdbHeader.age = 1;
    memcpy(pdbHeader.sig70, signature, sizeof(pdbHeader.sig70));

    auto newPdbHeaderStream = std::shared_ptr<MsfMemoryStream>(
            new MsfMemoryStream(pdbHeaderStream.get()));
    if (newPdbHeaderStream->write(sizeof(pdbHeader), &pdbHeader) !=
            sizeof(pdbHeader)) {
        throw InvalidPdb("failed to rewrite PDB header");
    }

    msf.replaceStream((size_t)PdbStreamType::header, newPdbHeaderStream);

    // Patch the DBI stream
    if (auto origDbiStream = msf.getStream((size_t)PdbStreamType::dbi)) {

        auto dbiStream = std::shared_ptr<MsfMemoryStream>(
                new MsfMemoryStream(origDbiStream.get()));

        patchDbiStream(dbiStream.get());

        msf.replaceStream((size_t)PdbStreamType::dbi, dbiStream);

        // We need the DBI header to get the symbol record stream. Note that bounds
        // checking has already been done at this point.
        const DbiHeader* dbiHeader = (const DbiHeader*)dbiStream->data();

        // Patch the symbol records stream
        if (auto origSymRecStream = msf.getStream(dbiHeader->symbolRecordsStream)) {
            auto symRecStream = std::shared_ptr<MsfMemoryStream>(
                    new MsfMemoryStream(origSymRecStream.get()));

            patchSymbolRecordsStream(symRecStream.get());

            msf.replaceStream(dbiHeader->symbolRecordsStream, symRecStream);
        }

        // Patch the public symbols info stream
        if (auto origPubSymStream =
                msf.getStream(dbiHeader->publicSymbolStream)) {
            auto pubSymStream = std::shared_ptr<MsfMemoryStream>(
                    new MsfMemoryStream(origPubSymStream.get()));

            patchPublicSymbolStream(pubSymStream.get());

            msf.replaceStream(dbiHeader->publicSymbolStream, pubSymStream);
        }
    }
}

/**
 * Patches a PDB file.
 */
template<typename CharT>
void patchPDB(const CharT* pdbPath, const CV_INFO_PDB70* pdbInfo,
        uint32_t timestamp, const uint8_t signature[16], bool dryrun) {

    auto tmpPdbPath = getTempPdbPath(pdbPath);

    {
        auto pdb = openFile(pdbPath, FileMode<CharT>::readExisting);
        auto tmpPdb = openFile(tmpPdbPath.c_str(), FileMode<CharT>::writeEmpty);

        MsfFile msf(pdb);

        patchPDB(msf, pdbInfo, timestamp, signature);

        // Write out the rewritten PDB to disk.
        msf.write(tmpPdb);
    }

    if (dryrun) {
        // Delete the temporary file
        deleteFile(tmpPdbPath.c_str());
    } else {
        // Rename the new PDB file over the old one
        renameFile(tmpPdbPath.c_str(), pdbPath);
    }
}

template<typename CharT>
void patchImageImpl(const CharT* imagePath, const CharT* pdbPath, bool dryrun) {
    MemMap image(imagePath);

    uint8_t* buf = (uint8_t*)image.buf();
    const size_t length = image.length();

    PEFile pe = PEFile(buf, length);

    Patches patches(buf);

    patches.add(&pe.fileHeader->TimeDateStamp, &pe.timestamp,
            "IMAGE_FILE_HEADER.TimeDateStamp");

    const CV_INFO_PDB70* pdbInfo = NULL;

    switch (pe.magic()) {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC: {
            // Patch as a PE32 file
            auto opt = pe.optionalHeader<IMAGE_OPTIONAL_HEADER32>();
            pdbInfo = pe.pdbInfo(opt);
            patchOptionalHeader(pe, patches, opt);
            break;
        }

        case IMAGE_NT_OPTIONAL_HDR64_MAGIC: {
            // Patch as a PE32+ file
            auto opt = pe.optionalHeader<IMAGE_OPTIONAL_HEADER64>();
            pdbInfo = pe.pdbInfo(opt);
            patchOptionalHeader(pe, patches, opt);
            break;
        }

        default:
            throw InvalidImage("unsupported IMAGE_NT_HEADERS.OptionalHeader");
    }

    patches.sort();

    // Calculate the checksum of the PE file. Note that the checksum is stored
    // in the PDB signature. When the patches are applied, this checksum is what
    // will be set in the file.
    calculateChecksum(buf, length, patches.patches, pe.pdbSignature);

    // Patch the PDB file.
    if (pdbPath) {
        patchPDB(pdbPath, pdbInfo, pe.timestamp, pe.pdbSignature, dryrun);
    }

    patches.apply(dryrun);
}

}

#if defined(_WIN32) && defined(UNICODE)

void patchImage(const wchar_t* imagePath, const wchar_t* pdbPath, bool dryrun) {
    patchImageImpl(imagePath, pdbPath, dryrun);
}

#else

void patchImage(const char* imagePath, const char* pdbPath, bool dryrun) {
    patchImageImpl(imagePath, pdbPath, dryrun);
}

#endif
