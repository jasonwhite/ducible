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

#include "msf_file_stream.h"

#include <algorithm>
#include <system_error>

#include <iostream>

MsfFileStream::MsfFileStream(FileRef f, size_t pageSize, size_t length, const uint32_t* pages)
    : _f(f), _pageSize(pageSize), _pos(0), _length(length)
{
    _pages.assign(pages, pages + ::pageCount(pageSize, length));
}

MsfFileStream::~MsfFileStream() {
    //std::cout << "Destroying MsfFileStream" << std::endl;
}

size_t MsfFileStream::length() const {
    return _length;
}

size_t MsfFileStream::getPos() const {
    return _pos;
}

void MsfFileStream::setPos(size_t pos) {
    _pos = pos;
}

size_t MsfFileStream::readFromPage(size_t page, size_t length, void* buf,
        size_t offset) {

    // Seek to the desired offset in the file.
    if (fseek(_f.get(), (long)(_pageSize * page + offset), SEEK_SET) != 0) {
        throw std::system_error(errno, std::system_category(),
                "Failed to seek to MSF page");
    }

    return fread(buf, 1, length, _f.get());
}

size_t MsfFileStream::read(size_t length, void* buf) {

    size_t bytesRead = 0;

    while (length > 0) {
        size_t i = _pos / _pageSize;
        size_t offset = _pos % _pageSize;
        size_t chunkSize = std::min(length, _pageSize - offset);

        if (i >= _pages.size())
            break;

        size_t chunkRead = readFromPage(_pages[i], chunkSize, buf, offset);
        bytesRead += chunkRead;

        _pos += chunkRead;

        if (chunkRead != chunkSize)
            break;

        length -= chunkSize;
        buf = (uint8_t*)buf + chunkSize;
    }

    return bytesRead;
}

size_t MsfFileStream::read(void* buf) {
    return read(_length - _pos, buf);
}

size_t MsfFileStream::write(size_t length, const void* buf) {
    // TODO
    return 0;
}
