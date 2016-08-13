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

#include <stdlib.h>

/**
 * Helper function for computing the number of pages required to hold a length
 * of bytes.
 */
template<typename T>
inline T pageCount(T pageSize, T length) {
    return (length + pageSize - 1) / pageSize;
}

/**
 * Represents an MSF stream.
 *
 * An MSF stream is made up of 1 or more pages. This class abstracts away the
 * task of reading from a stream to make it seem as if the data is sequential.
 */
class MsfStream {
public:
    /**
     * Returns the length of the stream, in bytes.
     */
    virtual size_t length() const = 0;

    /**
     * Gets the current position, in bytes, in the stream.
     */
    virtual size_t getPos() const = 0;

    /**
     * Sets the current position, in bytes, in the stream.
     */
    virtual void setPos(size_t p) = 0;

    /**
     * Reads a length of the stream. This abstracts reading from multiple pages.
     *
     * Params:
     *   length = The number of bytes to read from the stream.
     *   buf    = The buffer to read the stream into.
     *
     * Returns: The number of bytes read.
     */
    virtual size_t read(size_t length, void* buf) = 0;

    /**
     * Reads the entire stream.
     *
     * Params:
     *   buf = The buffer to read the stream into. This must be large enough to
     *         hold the entire stream.
     *
     * Returns: The number of bytes read.
     */
    virtual size_t read(void* buf) = 0;
};
