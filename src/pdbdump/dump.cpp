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

#include "pdbdump/dump.h"

#include "util/file.h"
#include "msf/msf.h"
#include "msf/stream.h"
#include "msf/file_stream.h"

#include "pdb/format.h"
#include "pdb/pdb.h"

namespace {

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
        throw InvalidPdb("wat");

    auto nameMap = readNameMapTable(buf.get(), buf.get()+remaining);

    for (auto const& kv: nameMap)
        os << kv.first << " => " << kv.second << std::endl;

    os << std::endl;
}

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
    const size_t streamCount = msf.streamCount();

    os << "Streams (" << streamCount << "):\n";

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

void dumpPdb(MsfFile& msf) {
    printStreamTable(msf, std::cout);
    printPdbStream(msf, std::cout);
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
