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

#include "msf.h"

#include <iostream>
#include <system_error>
#include <cstring>

#include "msf_stream.h"

namespace {

/**
 * Helper function for finding the size of the given file.
 */
int64_t getFileSize(FILE* f) {

    // Save our spot...
    int64_t pos = ftell(f);
    if (pos == -1) {
        throw std::system_error(errno, std::system_category(),
            "ftell() failed");
    }

    // Go to the end
    if (fseek(f, 0, SEEK_END) == -1) {
        throw std::system_error(errno, std::system_category(),
            "fseek() failed");
    }

    // Current position is the size of the file.
    int64_t size = ftell(f);
    if (size == -1) {
        throw std::system_error(errno, std::system_category(),
            "ftell() failed");
    }

    // Go back to our old spot
    if (fseek(f, (long)pos, SEEK_SET) == -1) {
        throw std::system_error(errno, std::system_category(),
            "fseek() failed");
    }

    return size;
}

}

MsfFile::MsfFile(FILE* f) {

    // Read the header
    if (fread(&_header, sizeof(_header), 1, f) != 1)
        throw InvalidMsf("Missing MSF header");

    // Check that this is indeed an MSF header
    if (memcmp(_header.magic, kMsfHeaderMagic, sizeof(kMsfHeaderMagic)) != 0)
        throw InvalidMsf("Invalid MSF header");

    // Check that the file size makes sense
    if (_header.pageSize * _header.pageCount != getFileSize(f))
        throw InvalidMsf("Invalid MSF file length");

    std::cout << "MSF Header info:\n";
    std::cout << "  Page Size:         " << _header.pageSize << std::endl;
    std::cout << "  Free Page Map:     " << _header.freePageMap << std::endl;
    std::cout << "  Page Count:        " << _header.pageCount << std::endl;
    std::cout << "  Stream Table Size: " << _header.streamTableInfo.size << std::endl;

    // The number of pages required to store the pages of the stream table
    // stream.
    size_t stPagesPagesCount =
        pageCount(_header.pageSize, _header.streamTableInfo.size);

    // Read the stream table page directory
    std::unique_ptr<uint32_t> streamTablePagesPages(
        new uint32_t[stPagesPagesCount]);

    if (fread(streamTablePagesPages.get(), sizeof(uint32_t), stPagesPagesCount, f) !=
            stPagesPagesCount) {
        throw InvalidMsf("Missing root MSF stream table page list");
    }

    MsfStream streamTablePagesStream(_header.pageSize, stPagesPagesCount * sizeof(uint32_t),
            streamTablePagesPages.get());

    // Read the list of stream table pages.
    std::vector<uint32_t> streamTablePages(stPagesPagesCount);
    streamTablePagesStream.read(f, &streamTablePages[0]);

    // Finally, read the stream table itself
    MsfStream streamTableStream(_header.pageSize, _header.streamTableInfo.size,
            &streamTablePages[0]);
    std::vector<uint32_t> streamTable(_header.streamTableInfo.size / sizeof(uint32_t));
    streamTableStream.read(f, &streamTable[0]);

    // The first element in the stream table is the total number of streams.
    const uint32_t& streamCount = streamTable[0];

    // The sizes of each stream then follow.
    const uint32_t* streamSizes = &streamTable[1];

    // After all the sizes are the lists of pages for each stream. We calculate
    // the number of pages required for the stream using the stream size.
    const uint32_t* streamPages = streamSizes + streamCount;

    uint32_t pagesIndex = 0;
    for (uint32_t i = 0; i < streamCount; ++i) {

        // If we were given a bogus stream count, we could potentially overflow
        // the stream table vector. Detect that here.
        if (pagesIndex >= streamTable.size())
            throw InvalidMsf("invalid stream count in stream table");

        const uint32_t& size = streamSizes[i];

        addStream(new MsfStream(_header.pageSize, size,
                streamPages + pagesIndex));

        pagesIndex += pageCount(_header.pageSize, size);
    }
}

size_t MsfFile::addStream(const MsfStream* stream) {
    _streams.push_back(std::shared_ptr<const MsfStream>(stream));
    return _streams.size()-1;
}

void MsfFile::replaceStream(size_t index, const MsfStream* stream) {
    _streams[index] = std::shared_ptr<const MsfStream>(stream);
}

size_t MsfFile::streamCount() const {
    return _streams.size();
}

/*void MsfFile::write(FILE* f) const {
    // TODO
}*/
