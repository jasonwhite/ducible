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

#include "msf_stream.h"

#include <algorithm>
#include <system_error>

MsfStream::MsfStream(size_t pageSize, size_t length, const uint32_t* pages)
    : _pageSize(pageSize), _length(length)
{
    _pages.assign(pages, pages + pageCount());
}

size_t MsfStream::length() const {
    return _length;
}

size_t MsfStream::pageSize() const {
    return _pageSize;
}

size_t MsfStream::pageCount() const {
    return ::pageCount(_pageSize, _length);
}

void MsfStream::readFromPage(FILE* f, size_t page, size_t length, void* buf,
        size_t offset) const {

    // Seek to the desired offset in the file.
    if (fseek(f, (long)(_pageSize * page + offset), SEEK_SET) != 0) {
        throw std::system_error(errno, std::system_category(),
                "Failed to seek to MSF page");
    }

    // Read from the page
    if (fread(buf, 1, length, f) != length) {
        throw std::system_error(errno, std::system_category(),
                "Failed to read from MSF page");
    }
}

void MsfStream::read(FILE* f, size_t length, void* buf, size_t pos) const {
    while (length > 0) {
        size_t i = pos / _pageSize;
        size_t offset = pos % _pageSize;
        size_t chunkSize = std::min(length, _pageSize - offset);
        readFromPage(f, _pages[i], chunkSize, buf, offset);

        length -= chunkSize;
        pos += chunkSize;
        buf = (uint8_t*)buf + chunkSize;
    }
}

void MsfStream::read(FILE* f, void* buf, size_t pos) const {
    read(f, _length - pos, buf, pos);
}
