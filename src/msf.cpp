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
    if (fseek(f, pos, SEEK_SET) == -1) {
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

    // Read the list of stream table pages.

}

size_t MsfFile::addStream(const MsfStream* stream) {
    _streams.push_back(stream);
    return _streams.size()-1;
}

void MsfFile::replaceStream(size_t index, const MsfStream* stream) {
    _streams[index] = stream;
}

size_t MsfFile::streamCount() const {
    return _streams.size();
}

void MsfFile::write(FILE* f) const {
    // TODO
}
