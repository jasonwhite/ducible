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

#include <algorithm>
#include <cstring>

#include "msf/readonly_stream.h"

MsfReadOnlyStream::MsfReadOnlyStream(size_t length, const void* buf)
    : _pos(0), _length(length), _data((const uint8_t*)buf)
{
}

size_t MsfReadOnlyStream::length() const {
    return _length;
}

size_t MsfReadOnlyStream::getPos() const {
    return _pos;
}

void MsfReadOnlyStream::setPos(size_t pos) {
    // Don't allow setting the position past the end of the stream.
    _pos = std::min(_length, pos);
}

size_t MsfReadOnlyStream::read(size_t length, void* buf) {

    if (_pos >= _length)
        return 0;

    size_t available = std::min(_length - _pos, length);

    memcpy(buf, _data + _pos, available);

    _pos += available;

    return available;
}

size_t MsfReadOnlyStream::read(void* buf) {
    return read(_length - _pos, buf);
}

size_t MsfReadOnlyStream::write(size_t length, const void* buf) {

    // TODO: Throw an exception instead
    (void)length;
    (void)buf;

    return 0;
}
