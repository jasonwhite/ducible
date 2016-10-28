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

#include <iostream>
#include <iomanip>
#include <memory>
#include <vector>
#include <cstring>

#include "pdbdump/dump.h"

#include "util/file.h"
#include "msf/msf.h"
#include "msf/stream.h"
#include "msf/file_stream.h"
#include "msf/memory_stream.h"

#include "pdb/format.h"
#include "pdb/pdb.h"

namespace {

void printLinkInfoStream(MsfMemoryStream* stream, std::ostream& os);

/**
 * Prints a nicely formatted page sequences (as if you were specifying the pages
 * to be printed).
 *
 * For example, the page list:
 *
 *    [0, 1, 2, 3, 4, 6, 7, 8, 9, 20]
 *
 * is printed as
 *
 *    [0-4, 6-9, 20]
 */
void printPageSequences(const std::vector<uint32_t>& pages, std::ostream& os) {
    os << "[";

    for (size_t i = 0; i < pages.size(); ) {

        if (i > 0)
            os << ", ";

        uint32_t start = pages[i];
        uint32_t count = 0;

        ++i;

        // Find how long a run of pages is.
        for (; i < pages.size() && pages[i] == pages[i-1]+1; ++i)
            ++count;

        if (count == 0) {
            os << start
               << " (0x" << std::hex
               << (uint64_t)start * 4096 << "-0x"
               << ((uint64_t)start+1) * 4096 - 1
               << ")" << std::dec;
        }
        else {
            os << start << "-" << start+count
               << " (0x" << std::hex
               << ((uint64_t)start) * 4096 << "-0x"
               << ((uint64_t)start+count+1) * 4096 - 1
               << ")" << std::dec;
        }
    }

    os << "]";
}

/**
 * Prints the stream table.
 */
void printStreamTable(MsfFile& msf, std::ostream& os) {

    os << "Stream Table\n"
       << "============\n";

    const size_t streamCount = msf.streamCount();

    for (size_t i = 0; i < streamCount; ++i) {
        auto stream = std::dynamic_pointer_cast<MsfFileStream>(msf.getStream(i));

        const auto& pages = stream->pages();

        os << std::setw(5) << i << ": "
           << std::setw(8) << stream->length() << " bytes, "
           << std::setw(4) << pages.size() << " pages ";

        printPageSequences(pages, os);

        std::cout << std::endl;
    }

    os << std::endl;
}

/**
 * Prints out a GUID to the given stream.
 */
void printGUID(uint8_t guid[16], std::ostream& os) {

    const auto flags = os.flags();

    os << std::hex << std::setfill('0') << std::setw(2) << std::uppercase;

    for (size_t i = 0; i < 4; ++i) os << (int)guid[i];
    os << "-";
    for (size_t i = 4; i < 6; ++i) os << (int)guid[i];
    os << "-";
    for (size_t i = 6; i < 8; ++i) os << (int)guid[i];
    os << "-";
    for (size_t i = 8; i < 10; ++i) os << (int)guid[i];
    os << "-";
    for (size_t i = 10; i < 16; ++i) os << (int)guid[i];

    // Restore flags
    os.flags(flags);
}

/**
 * Prints out information in the PDB stream.
 */
void printPdbStream(MsfFile& msf, std::ostream& os) {

    static const size_t streamid = (size_t)PdbStreamType::header;

    auto stream = msf.getStream(streamid);
    if (!stream)
        throw InvalidPdb("missing PDB header stream");

    os << "PDB Stream Info\n"
       << "===============\n";

    os << "Stream ID:   " << streamid << std::endl;
    os << "Stream Size: " << stream->length() << " bytes" << std::endl;
    os << std::endl;

    PdbStream70 header;

    if (stream->read(sizeof(header), &header) != sizeof(header))
        throw InvalidPdb("missing PDB 7.0 header");

    os << "Header\n"
       << "------\n";

    os << "Version:   " << (uint32_t)header.version << std::endl;
    os << "Timestamp: " << header.timestamp << std::endl;
    os << "Age:       " << header.age << std::endl;
    os << "Signature: "; printGUID(header.sig70, os); os << std::endl;
    os << std::endl;

    os << "Name Map Table\n"
       << "--------------\n";

    // Read the rest of the stream, it should contain only the name map.
    const size_t remaining = stream->length() - stream->getPos();
    std::unique_ptr<uint8_t> buf(new uint8_t[remaining]);
    if (stream->read(remaining, buf.get()) != remaining)
        throw InvalidPdb("failed to read name map table");

    auto nameMap = readNameMapTable(buf.get(), buf.get()+remaining);

    for (auto const& kv: nameMap)
        os << kv.first << " => " << kv.second << std::endl;

    os << std::endl;

    // Dump the /LinkInfo stream if it exists.
    const auto it = nameMap.find("/LinkInfo");
    if (it != nameMap.end()) {
        auto stream = msf.getStream(it->second);
        if (!stream)
            throw InvalidPdb("missing '/LinkInfo' stream");

        auto linkInfoStream = std::shared_ptr<MsfMemoryStream>(
                new MsfMemoryStream(stream.get()));

        printLinkInfoStream(linkInfoStream.get(), os);
    }
}

/**
 * Prints out information in the "/LinkInfo" stream.
 */
void printLinkInfoStream(MsfMemoryStream* stream, std::ostream& os) {
    os << "Link Info Stream\n"
       << "================\n";

    uint8_t* data = stream->data();
    const size_t length = stream->length();

    if (length == 0) return;

    const LinkInfo* linkInfo = (const LinkInfo*)data;

    if (length < sizeof(LinkInfo))
        throw InvalidPdb("got partial LinkInfo stream");

    if (linkInfo->size > length)
        throw InvalidPdb("LinkInfo size too large for stream");

    os << "CWD:         '" << linkInfo->cwd<char>()        << "'" << std::endl
       << "Command:     '" << linkInfo->command<char>()    << "'" << std::endl
       << "Libs:        '" << linkInfo->libs<char>()       << "'" << std::endl
       << "Output File: '" << linkInfo->outputFile<char>() << "'" << std::endl
       << std::endl;
}

/**
 * Prints out information in the DBI stream.
 */
void printDbiStream(MsfFile& msf, std::ostream& os) {

    static const size_t streamid = (size_t)PdbStreamType::dbi;

    auto stream = msf.getStream(streamid);
    if (!stream) return;

    os << "DBI Stream Info\n"
       << "===============\n";

    os << "Stream ID:   " << streamid << std::endl;
    os << "Stream Size: " << stream->length() << " bytes" << std::endl;
    os << std::endl;

    DbiHeader dbi;

    if (stream->read(sizeof(dbi), &dbi) != sizeof(dbi))
        throw InvalidPdb("missing DBI dbi");

    os << "Header\n"
       << "------\n";

    os << "Signature:                          0x" << std::hex << dbi.signature << std::dec <<
        std::endl
       << "Version:                            " << (uint32_t)dbi.version << std::endl
       << "Age:                                " << dbi.age << std::endl
       << "Global Symbol Info (GSI) Stream ID: " << dbi.globalSymbolStream << std::endl
       << "PDB DLL Version:                    "
           << dbi.pdbDllVersion.major << "."
           << dbi.pdbDllVersion.minor << "."
           << dbi.pdbDllVersion.format << std::endl
       << "Public Symbol Info (PSI) Stream ID: " << dbi.publicSymbolStream << std::endl
       << "PDB DLL Build Major Version:        " << dbi.pdbDllBuildVersionMajor << std::endl
       << "Symbol Records Stream ID:           " << dbi.symbolRecordsStream << std::endl
       << "PDB DLL Build Minor Version:        " << dbi.pdbDllBuildVersionMinor << std::endl
       << "Module Info Size:                   " << dbi.gpModInfoSize << std::endl
       << "Section Contribution Size:          " << dbi.sectionContributionSize << " bytes" << std::endl
       << "Section Map Size:                   " << dbi.sectionMapSize << " bytes" << std::endl
       << "File Info Size:                     " << dbi.fileInfoSize << " bytes" << std::endl
       << "Type Server Map Size:               " << dbi.typeServerMapSize << " bytes" << std::endl
       << "MFC Type Server Index:              " << dbi.mfcIndex << std::endl
       << "Debug Header Size:                  " << dbi.debugHeaderSize << std::endl
       << "EC Info Size:                       " << dbi.ecInfoSize << std::endl
       << "Flags:" << std::endl
       << "    Incrementally Linked:           " << (dbi.flags.incLink ? "yes" : "no") << std::endl
       << "    Stripped:                       " << (dbi.flags.stripped ? "yes" : "no") << std::endl
       << "    CTypes:                         " << (dbi.flags.ctypes ? "yes" : "no") << std::endl
       << "Machine Type:                       " << dbi.machine << std::endl
       << std::endl;

    size_t moduleCount = 0;

    {
        os << "Module Info\n"
           << "-----------\n";

        std::unique_ptr<uint8_t> modInfo(new uint8_t[dbi.gpModInfoSize]);
        if (stream->read(dbi.gpModInfoSize, modInfo.get()) != dbi.gpModInfoSize)
            throw InvalidPdb("failed to read module info sub-stream");

        // Print the module info
        for (size_t i = 0; i < dbi.gpModInfoSize; ) {
            if (dbi.gpModInfoSize - i < sizeof(ModuleInfo))
                throw InvalidPdb("got partial DBI module info");

            ModuleInfo* info = (ModuleInfo*)(modInfo.get() + i);

            os  << "Module ID:   " << moduleCount << std::endl
                << "Module Name: '" << info->moduleName() << "'" << std::endl
                << "Object Name: '" << info->objectName() << "'" << std::endl
                << "Stream ID:   " << info->stream << std::endl
                << std::endl;

            i += info->size();
            ++moduleCount;
        }
    }

    {
        os << "Section Contributions\n"
           << "---------------------\n";

        os << "Section Contribution Count: " <<
            dbi.sectionContributionSize / sizeof(SectionContribution) << std::endl;

        os << std::endl;

        // Skip over the section contribution
        stream->skip(dbi.sectionContributionSize);
    }

    {
        os << "Section Map\n"
           << "-----------\n";

        os << "No information available.\n";

        os << std::endl;

        // Skip over the section map
        stream->skip(dbi.sectionMapSize);
    }

    if (dbi.fileInfoSize > 0) {

        // These are files that correspond to each module as listed in the
        // Module Info substream above.

        os << "File Info\n"
           << "---------\n";

        std::unique_ptr<uint8_t> fileInfo(new uint8_t[dbi.fileInfoSize]);
        if (stream->read(dbi.fileInfoSize, fileInfo.get()) != dbi.fileInfoSize)
            throw InvalidPdb("failed to read file info sub-stream");

        const uint8_t* p = fileInfo.get();
        const uint8_t* pEnd = p + dbi.fileInfoSize;

        // Skip over the header as it doesn't always provide correct
        // information.
        p += sizeof(FileInfoHeader);

        // Skip over file indices array. We don't need them.
        p += moduleCount * sizeof(uint16_t);

        // File counts array
        const uint16_t* fileCounts = (const uint16_t*)p;
        p += moduleCount * sizeof(*fileCounts);

        if (p >= pEnd)
            throw InvalidPdb("got partial file info in DBI stream");

        const uint32_t* offsets = (const uint32_t*)p;

        uint32_t offsetCount = 0;
        for (size_t i = 0; i < moduleCount; ++i)
            offsetCount += fileCounts[i];

        p += offsetCount * sizeof(*offsets);

        if (p >= pEnd)
            throw InvalidPdb("got partial file info in DBI stream");

        const char* names = (char*)p;

        size_t offset = 0;

        for (size_t i = 0; i < moduleCount; ++i) {

            os << "Module " << i << std::endl;

            for (size_t j = 1; j < fileCounts[i]; ++j) {
                os << "    " << names + offsets[offset] << std::endl;
                ++offset;
            }

            os << std::endl;
        }
    }

    os << std::endl;

    {
        os << "Type Server Map (TSM)\n"
           << "---------------------\n";

        os << "No information available.\n";

        os << std::endl;

        // Skip over the TSM substream
        stream->skip(dbi.typeServerMapSize);
    }

    {
        os << "EC Info\n"
           << "-------\n";

        os << "No information available.\n";

        os << std::endl;

        // Skip over the EC info substream
        stream->skip(dbi.ecInfoSize);
    }

    {
        os << "Debug Header\n"
           << "------------\n";

        std::unique_ptr<uint8_t> debugHeader(new uint8_t[dbi.debugHeaderSize]);
        if (stream->read(dbi.debugHeaderSize, debugHeader.get()) != dbi.debugHeaderSize)
            throw InvalidPdb("failed to read DBI debug header");

        if (dbi.debugHeaderSize/sizeof(int16_t) < DebugTypes::count)
            throw InvalidPdb("got partial DBI debug header");

        const int16_t* streams = (const int16_t*)debugHeader.get();

        os << "fpo            = " << streams[DebugTypes::fpo]            << std::endl
           << "exception      = " << streams[DebugTypes::exception]      << std::endl
           << "fixup          = " << streams[DebugTypes::fixup]          << std::endl
           << "omapToSrc      = " << streams[DebugTypes::omapToSrc]      << std::endl
           << "omapFromSrc    = " << streams[DebugTypes::omapFromSrc]    << std::endl
           << "sectionHdr     = " << streams[DebugTypes::sectionHdr]     << std::endl
           << "tokenRidMap    = " << streams[DebugTypes::tokenRidMap]    << std::endl
           << "xdata          = " << streams[DebugTypes::xdata]          << std::endl
           << "pdata          = " << streams[DebugTypes::pdata]          << std::endl
           << "newFPO         = " << streams[DebugTypes::newFPO]         << std::endl
           << "sectionHdrOrig = " << streams[DebugTypes::sectionHdrOrig] << std::endl
           << std::endl;
    }
}

void dumpPdb(MsfFile& msf) {
    printStreamTable(msf, std::cout);
    printPdbStream(msf, std::cout);
    printDbiStream(msf, std::cout);
}

template<typename CharT>
void dumpPdbImpl(const CharT* path) {
    auto pdb = openFile(path, FileMode<CharT>::readExisting);

    MsfFile msf(pdb);

    dumpPdb(msf);
}

}

#if defined(_WIN32) && defined(UNICODE)

void dumpPdb(const wchar_t* path) {
    dumpPdbImpl(path);
}

#else

void dumpPdb(const char* path) {
    dumpPdbImpl(path);
}

#endif
