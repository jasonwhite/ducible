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
#pragma once

#include <stdint.h>

#include "msf/stream.h"

/**
 * A lightweight read-only memory stream.
 */
class MsfReadOnlyStream : public MsfStream {
private:

    size_t _pos;
    size_t _length;
    const uint8_t* _data;

public:
    /**
     * Initialize the stream with an buffer. The buffer is copied internally.
     *
     * Params:
     *   length = Length of the buffer, in bytes.
     *   buf    = The buffer.
     */
    MsfReadOnlyStream(size_t length, const void* buf);

    /**
     * Returns the length of the stream, in bytes.
     */
    size_t length() const;

    /**
     * Returns a pointer to the underlying data.
     */
    const uint8_t* data() const {
        return _data;
    }

    /**
     * Gets the current position, in bytes, in the stream.
     */
    size_t getPos() const;

    /**
     * Sets the current position, in bytes, in the stream.
     */
    void setPos(size_t p);

    /**
     * Reads a length of the stream. This abstracts reading from multiple pages.
     *
     * Params:
     *   length = The number of bytes to read from the stream.
     *   buf    = The buffer to read the stream into.
     *
     * Returns: The number of bytes read.
     */
    size_t read(size_t length, void* buf);

    /**
     * Reads the entire stream.
     *
     * Params:
     *   buf = The buffer to read the stream into. This must be large enough to
     *         hold the entire stream.
     *
     * Returns: The number of bytes read.
     */
    size_t read(void* buf);

    /**
     * Writes a buffer to the stream from the current position. If attempting to
     * write past the end of the stream, the length of the stream will grow.
     */
    size_t write(size_t length, const void* buf);
};

