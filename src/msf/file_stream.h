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
#include <vector>

#include "msf/stream.h"
#include "util/file.h"

/**
 * Represents an MSF file stream.
 */
class MsfFileStream : public MsfStream {
private:

    FileRef _f;
    size_t _pageSize;
    size_t _pos;
    size_t _length;
    std::vector<uint32_t> _pages;

public:
    /**
     * Params:
     *   f        = FILE pointer.
     *   pageSize = Length of one page, in bytes.
     *   length   = Length of the stream, in bytes.
     *   pages    = List of pages. The length of this array is calculated using
     *              the page size and stream length.
     */
    MsfFileStream(FileRef f, size_t pageSize, size_t length, const uint32_t* pages);

    /**
     * Returns the length of the stream, in bytes.
     */
    size_t length() const;

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
     * Writes a buffer to the stream from the current position. If an attempt is
     * made to write past the end of the last page, it will only partially
     * succeed. No new pages will be allocated.
     */
    size_t write(size_t length, const void* buf);

private:

    /**
     * Reads a single page from the stream.
     *
     * Params:
     *   f    = The file to read from. The seek location is not guaranteed to be
     *          the same after this function is finished.
     *   page = The page number to read from.
     *   length = The number of bytes to read from the page.
     *   buf  = The buffer to read the page into.
     *
     * Returns: The number of bytes read.
     */
    size_t readFromPage(size_t page, size_t length, void* buf, size_t offset = 0);
};
