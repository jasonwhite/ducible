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

#include "msf_memory_stream.h"

MsfMemoryStream::MsfMemoryStream(size_t length, const void* buf)
    : _pos(0)
{
    _data.resize(length);
    memcpy(&_data[0], buf, length);
}

MsfMemoryStream::MsfMemoryStream(MsfStream* stream)
    : _pos(0)
{
    const size_t length = stream->length();

    _data.resize(length);

    const size_t pos = stream->getPos();
    stream->setPos(0);

    stream->read(length, &_data[0]);

    stream->setPos(pos);
}

size_t MsfMemoryStream::length() const {
    return _data.size();
}

size_t MsfMemoryStream::getPos() const {
    return _pos;
}

void MsfMemoryStream::setPos(size_t pos) {
    // Don't allow setting the position past the end of the stream.
    _pos = std::min(_data.size(), pos);
}

size_t MsfMemoryStream::read(size_t length, void* buf) {

    if (_pos >= _data.size())
        return 0;

    size_t available = std::min(_data.size() - _pos, length);

    memcpy(buf, &_data[0], available);

    _pos += available;

    return available;
}

size_t MsfMemoryStream::read(void* buf) {
    return read(_data.size() - _pos, buf);
}

size_t MsfMemoryStream::write(size_t length, const void* buf) {

    const size_t available = _data.size() - _pos;

    // Not enough room, need to grow the stream.
    if (available < length)
        _data.resize(_pos + length);

    memcpy(&_data[_pos], buf, length);

    _pos += length;

    return length;
}
