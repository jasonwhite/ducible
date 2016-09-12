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
 * - http://llvm.org/svn/llvm-project/llvm/trunk/lib/DebugInfo/PDB/Raw/
 * - https://github.com/Microsoft/microsoft-pdb (Microsoft's reference
 *   implementation for reading/writing PDBs.)
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>
#include <map>
#include <regex>

#include "patch_image.h"

#include "patches.h"
#include "pe_file.h"
#include "file.h"

#include "msf.h"
#include "msf_stream.h"
#include "msf_memory_stream.h"
#include "pdb.h"

#include "cvinfo.h"

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

using NameMapTable = std::map<std::string, uint32_t>;

/**
 * Reads the name map table in the PDB header stream. This is a map of strings
 * to stream numbers.
 *
 * The format is as follows:
 *
 *  1. String buffer:
 *     (a) stringsLength (4 bytes): The size of the string buffer
 *     (b) strings (stringsLength bytes): A list of null-terminated strings.
 *  2. The map of strings to stream indices:
 *     (a) elemCount (4 byte): The number of items in the map (aka its
 *         cardinality).
 *     (b) elemCountMax (4 bytes): The capacity of the map.
 *     (c) Bitset of present elements. This keeps track of which 'holes' have
 *         been filled in the map. There should be elemCount bits set in this
 *         bitset.
 *         i. count (4 bytes): The number of elements in the bitset
 *         ii. bitset (count * 4 bytes): The bits
 *     (c) Bitmap of deleted elements
 *         i. count (4 bytes): The number of elements in the bitset
 *         ii. bitset (count * 4 bytes): The bits
 *     (d) A list of elemCount (string offset, stream index) pairs.
 *
 * Microsoft's PDB implementation was used as a reference. More specifically,
 * see the following files:
 *
 *  1. PDB/include/nmtni.h - NMTNI::reload() - for loading the name table from
 *     disk, which includes a Map.
 *  2. PDB/include/map.h - Map::reload() - for loading a Map from disk.
 *  3. PDB/include/iset.h - ISet::reload() - for loading a bitset from disk,
 *     which is just an Array of longs.
 */
NameMapTable readNameMapTable(const uint8_t* data, const uint8_t* dataEnd) {
    NameMapTable table;

    // Parse the name map
    if (size_t(dataEnd - data) < sizeof(uint32_t))
        throw InvalidPdb("missing PDB name table strings length");

    const uint32_t stringsLength = *(const uint32_t*)data;
    data += sizeof(stringsLength);

    if (size_t(dataEnd - data) < stringsLength)
        throw InvalidPdb("missing PDB name table strings data");

    // The names of the streams. We'll index into this later.
    const char* strings = (const char*)data;
    data += stringsLength;

    if (size_t(dataEnd - data) < 2 * sizeof(uint32_t))
        throw InvalidPdb("missing PDB stream name map sizes");

    // The number of elements in the hash table.
    const uint32_t elemCount = *(const uint32_t*)data;
    data += sizeof(elemCount);

    // The maximum number of elements in the hash table.
    const uint32_t elemCountMax = *(const uint32_t*)data;
    data += sizeof(elemCountMax);

    if (size_t(dataEnd - data) < sizeof(uint32_t))
        throw InvalidPdb("missing PDB name table 'present' bitset size");

    // Skip over the "present" bitset.
    const uint32_t presentSize = *(const uint32_t*)data;
    data += sizeof(presentSize);
    data += presentSize * sizeof(uint32_t);

    if (data > dataEnd)
        throw InvalidPdb("missing PDB name table 'present' bitset data");

    if (size_t(dataEnd - data) < sizeof(uint32_t))
        throw InvalidPdb("missing PDB name table 'deleted' bitset size");

    // Skip over the "deleted" bitset.
    const uint32_t deletedSize = *(const uint32_t*)data;
    data += sizeof(deletedSize);
    data += deletedSize * sizeof(uint32_t);

    if (data > dataEnd)
        throw InvalidPdb("missing PDB name table 'deleted' bitset data");

    if (size_t(dataEnd - data) < elemCount * sizeof(uint32_t) * 2)
        throw InvalidPdb("missing PDB name table pairs");

    // Finally, read the pairs of string offsets and stream indices
    const uint32_t* pairs = (const uint32_t*)data;
    for (size_t i = 0; i < elemCount; ++i) {
        const uint32_t offset = pairs[i*2];

        if (offset >= stringsLength)
            throw InvalidPdb("invalid PDB name table offset into strings buffer");

        const char* name = &strings[offset];
        const uint32_t stream = pairs[i*2+1];
        table[std::string(name)] = stream;
    }

    return table;
}

/**
 * Patches the "/LinkInfo" named stream.
 */
void patchLinkInfoStream(MsfMemoryStream* stream) {
    uint8_t* data = stream->data();
    const size_t length = stream->length();

    if (length == 0)
        return;

    if (length < sizeof(LinkInfo))
        throw InvalidPdb("got partial LinkInfo stream");

    const LinkInfo* linkInfo = (LinkInfo*)data;

    if (linkInfo->size > length)
        throw InvalidPdb("LinkInfo size too large for stream");

    // The rest of the stream appears to be garbage. Thus, we truncate it.
    stream->resize(linkInfo->size);
}

/**
 * Patches the "/names" stream.
 */
void patchNamesStream(MsfMemoryStream* stream) {
    uint8_t* data = stream->data();
    uint8_t* dataEnd = data + stream->length();

    // Parse the header
    if (size_t(dataEnd - data) < sizeof(StringTableHeader))
        throw InvalidPdb("missing string table header");

    StringTableHeader* header = (StringTableHeader*)data;

    data += sizeof(*header);

    if (header->signature != kHashTableSignature)
        throw InvalidPdb("got invalid string table signature");

    if (header->version != 1 && header->version != 2)
        throw InvalidPdb("got invalid or unsupported string table version");

    if (size_t(dataEnd - data) < header->stringsSize)
        throw InvalidPdb("got partial string table data");

    data += header->stringsSize;

    if (size_t(dataEnd - data) < sizeof(uint32_t))
        throw InvalidPdb("missing string table offset array length");

    // Offsets array length
    uint32_t offsetsLength = *(uint32_t*)data;

    data += sizeof(offsetsLength);

    if (size_t(dataEnd - data) < offsetsLength * sizeof(uint32_t))
        throw InvalidPdb("got partial string table offsets array");

    uint32_t* offsets = (uint32_t*)data;

    data += offsetsLength * sizeof(uint32_t);

    // Sort the offsets. There is some non-determinism creeping in here somehow.
    std::sort(offsets, offsets + offsetsLength);

    for (size_t i = 0; i < offsetsLength; ++i) {
        const size_t offset = offsets[i];

        if (offset == 0)
            continue;

        if (offset >= header->stringsSize)
            throw InvalidPdb("got invalid offset into string table");

        char* str = &header->strings[offset];
        size_t len = strlen(str);

        if (offset + len + 1 > header->stringsSize)
            throw InvalidPdb("got invalid offset into string table");

        normalizeFileNameGuid(str, len);
    }
}

/**
 * Patches the PDB header stream.
 */
void patchHeaderStream(MsfFile& msf, MsfMemoryStream* stream, const CV_INFO_PDB70* pdbInfo,
        uint32_t timestamp, const uint8_t signature[16]) {

    uint8_t* data = stream->data();
    const uint8_t* dataEnd = stream->data() + stream->length();

    if (size_t(dataEnd - data) < sizeof(PdbStream70))
        throw InvalidPdb("missing PDB 7.0 header");

    PdbStream70* header = (PdbStream70*)data;

    data += sizeof(*header);

    if (header->version < PdbVersion::vc70)
        throw InvalidPdb("unsupported PDB implementation version");

    // Check that this PDB matches what the PE file expects
    if (!pdbInfo || !matchingSignatures(*pdbInfo, *header))
        throw InvalidPdb("PE and PDB signatures do not match");

    // Patch the PDB header stream
    header->timestamp = timestamp;
    header->age = 1;
    memcpy(header->sig70, signature, sizeof(header->sig70));

    const auto table = readNameMapTable(data, dataEnd);

    // Patch the LinkInfo stream.
    {
        const auto it = table.find("/LinkInfo");
        if (it != table.end()) {
            auto origLinkInfoStream = msf.getStream(it->second);
            if (!origLinkInfoStream)
                throw InvalidPdb("missing '/LinkInfo' stream");

            auto linkInfoStream = std::shared_ptr<MsfMemoryStream>(
                    new MsfMemoryStream(origLinkInfoStream.get()));

            patchLinkInfoStream(linkInfoStream.get());

            msf.replaceStream(it->second, linkInfoStream);
        }
    }

    // Rewrite /names hash table
    {
        const auto it = table.find("/names");
        if (it != table.end()) {
            auto origNamesStream = msf.getStream(it->second);
            if (!origNamesStream)
                throw InvalidPdb("missing '/names' stream");

            auto namesStream = std::shared_ptr<MsfMemoryStream>(
                    new MsfMemoryStream(origNamesStream.get()));

            patchNamesStream(namesStream.get());

            msf.replaceStream(it->second, namesStream);
        }
    }
}

/**
 * Patches a module stream.
 */
void patchModuleStream(MsfMemoryStream* stream) {

    uint8_t* data = stream->data();
    const uint8_t* dataEnd = stream->data() + stream->length();

    if (size_t(dataEnd - data) < sizeof(uint16_t))
        throw InvalidPdb("got partial module info stream");

    uint32_t type = *(uint32_t*)data;
    data += sizeof(type);

    if (type != CV_SIGNATURE_C13)
        return;

    if (size_t(dataEnd - data) < sizeof(SymbolRecord))
        throw InvalidPdb("missing symbol record in module info stream");

    const SymbolRecord* sym = (const SymbolRecord*)data;

    // We're only concerned about objects here
    if (sym->type != S_OBJNAME)
        return;

    // Recast now that we know the type.
    OBJNAMESYM* objsym = (OBJNAMESYM*)data;

    // The signature always seems to be 0.
    if (objsym->signature != 0)
        throw InvalidPdb("got invalid OBJNAMESYM symbol record signature");

    if (size_t(dataEnd - data) < objsym->reclen)
        throw InvalidPdb("got partial OBJNAMESYM symbol record");

    size_t namelen = strlen((const char*)objsym->name);

    if ((uint8_t*)objsym->name + namelen + 1 > dataEnd)
        throw InvalidPdb("object path in symbol record is not null-terminated");

    normalizeFileNameGuid((char*)objsym->name, namelen);
}

/**
 * Patches the DBI stream.
 */
void patchDbiStream(MsfFile& msf, MsfMemoryStream* stream) {

    if (stream->length() < sizeof(DbiHeader))
        throw InvalidPdb("DBI stream too short");

    uint8_t* data = stream->data();
    const size_t length = stream->length();
    size_t offset = 0;

    DbiHeader* dbi = (DbiHeader*)data;

    // Sanity checks
    if (dbi->signature != dbiHeaderSignature)
        throw InvalidPdb("invalid DBI header signature");

    if (dbi->version != DbiVersion::v70)
        throw InvalidPdb("Unsupported DBI stream version");

    // Patch the age. This must match the age in the PDB stream.
    dbi->age = 1;

    offset += sizeof(*dbi);

    // The module info immediately follows the header.

    // Check bounds
    if (offset + dbi->gpModInfoSize > length)
        throw InvalidPdb("DBI module info size exceeds stream length");

    // Number of modules
    size_t moduleCount = 0;

    // Patch the module info entries
    for (size_t i = 0; i < dbi->gpModInfoSize; ) {
        if (dbi->gpModInfoSize - i < sizeof(ModuleInfo))
            throw InvalidPdb("got partial DBI module info");

        ModuleInfo* info = (ModuleInfo*)(data + offset + i);

        info->sc.padding1 = 0;
        info->sc.padding2 = 0;

        // Patch the offsets "array". This is not used directly by Microsoft's
        // DBI implementation and may contain non-deterministic data (e.g., the
        // memory address of the actual allocated array). Thus, we need to zero
        // it out.
        info->offsets = 0;

        // There is one entry that contains a path with a GUID. We need to patch
        // this. It is often the first module info entry, but it is safer to
        // find it by name.
        if (strcmp(info->moduleName(), "* Linker Generated Manifest RES *") == 0 &&
            strcmp(info->objectName(), "") == 0) {

            auto origModuleStream = msf.getStream(info->stream);
            if (!origModuleStream)
                continue;

            auto moduleStream = std::shared_ptr<MsfMemoryStream>(
                    new MsfMemoryStream(origModuleStream.get()));

            patchModuleStream(moduleStream.get());

            msf.replaceStream(info->stream, moduleStream);
        }

        i += info->size();
        ++moduleCount;
    }

    offset += dbi->gpModInfoSize;

    // The section contributions follow the module info entries. These contain
    // garbage due to struct alignment. They needed to be zeroed out.

    if (offset + dbi->sectionContributionSize > length) {
        throw InvalidPdb(
                "DBI section contributions size exceeds stream length");
    }

    const size_t scCount = dbi->sectionContributionSize /
        sizeof(SectionContribution);

    SectionContribution* sectionContribs = (SectionContribution*)(data + offset);

    for (size_t i = 0; i < scCount; ++i) {
        SectionContribution& sc = sectionContribs[i];
        sc.padding1 = 0;
        sc.padding2 = 0;
    }

    offset += dbi->sectionContributionSize;

    // Skip over the section map
    offset += dbi->sectionMapSize;

    // In the list of files, there are some temporary files with random GUIDs in
    // the name.
    if (dbi->fileInfoSize > 0) {

        if (offset + dbi->fileInfoSize > length)
            throw InvalidPdb("Missing file info in DBI stream");

        uint8_t* p = data + offset;
        uint8_t* pEnd = p + dbi->fileInfoSize;

        // Skip over the header as it doesn't always provide correct
        // information.
        p += sizeof(FileInfoHeader);

        // Skip over file indices array. We don't need them.
        p += moduleCount * sizeof(uint16_t);

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
            const uint32_t& off = offsets[i];

            if ((uint8_t*)names + off + 1 > pEnd)
                throw InvalidPdb("invalid offset for file info name");

            char* name = names + off;
            size_t len = strlen(name);

            if ((uint8_t*)name + len + 1 > pEnd)
                throw InvalidPdb("file name exceeds file info section size");

            normalizeFileNameGuid(name, len);
        }
    }

    // Skip past the file info
    offset += dbi->fileInfoSize;

    // Skip past the TSM substream
    offset += dbi->typeServerMapSize;

    // Skip past the EC info
    offset += dbi->ecInfoSize;

    // Skip past the debug header. This should be the last substream in the DBI
    // stream.
    offset += dbi->debugHeaderSize;
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
    auto origPdbHeaderStream = msf.getStream((size_t)PdbStreamType::header);
    if (!origPdbHeaderStream)
        throw InvalidPdb("missing PDB header stream");

    auto pdbHeaderStream = std::shared_ptr<MsfMemoryStream>(
            new MsfMemoryStream(origPdbHeaderStream.get()));

    patchHeaderStream(msf, pdbHeaderStream.get(), pdbInfo, timestamp, signature);

    msf.replaceStream((size_t)PdbStreamType::header, pdbHeaderStream);

    // Patch the DBI stream
    if (auto origDbiStream = msf.getStream((size_t)PdbStreamType::dbi)) {

        auto dbiStream = std::make_shared<MsfMemoryStream>(
                origDbiStream.get());

        patchDbiStream(msf, dbiStream.get());

        msf.replaceStream((size_t)PdbStreamType::dbi, dbiStream);

        // We need the DBI header to get the symbol record stream. Note that bounds
        // checking has already been done at this point.
        const DbiHeader* dbiHeader = (const DbiHeader*)dbiStream->data();

        // Patch the symbol records stream
        if (auto origSymRecStream = msf.getStream(dbiHeader->symbolRecordsStream)) {
            auto symRecStream = std::make_shared<MsfMemoryStream>(
                    origSymRecStream.get());

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

// Helpers for CharT generalization
template<typename CharT>
constexpr CharT ilkExtension[] = {};

template<> constexpr char ilkExtension<char>[] = ".ilk";
template<> constexpr wchar_t ilkExtension<wchar_t>[] = L".ilk";

/**
 * Patches the PDB signature in the .ilk file so that incremental linking still
 * works.
 */
template<typename CharT>
void patchIlk(const CharT* imagePath, const uint8_t oldSignature[16],
        const uint8_t newSignature[16], bool dryrun) {

    (void)oldSignature;
    (void)newSignature;
    (void)dryrun;

    std::basic_string<CharT> ilkPath(imagePath);
    size_t extpos = ilkPath.find_last_of('.');

    // Strip off the extension.
    if (extpos != std::basic_string<CharT>::npos)
        ilkPath.resize(extpos);

    ilkPath.append(ilkExtension<CharT>);

    // Map the ilk file into memory.
    try {
        MemMap ilk(ilkPath.c_str());

        uint8_t* buf = (uint8_t*)ilk.buf();
        uint8_t* bufEnd = buf + ilk.length();

        // Find
        uint8_t* it = std::find_first_of(buf, bufEnd,
                oldSignature, oldSignature+16);

        // Replace
        if (it != bufEnd) {
            std::cout << "Replacing old PDB signature in ILK file.\n";
            memcpy(it, newSignature, 16);
        }
    }
    catch (const std::system_error&) {
        // Ignore.
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

    // Patch the ilk file with the new PDB signature. If we don't do this,
    // incremental linking will fail due to a signature mismatch.
    if (pdbInfo) {
        patchIlk(imagePath, pdbInfo->Signature, pe.pdbSignature, dryrun);
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
