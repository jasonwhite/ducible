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

struct STREAM_INFO {
    // Size of the stream, in bytes
    uint32_t size;

    // Index of the page number.
    int32_t index;
};

struct MSF_HEADER {
    // Magic version string. Used to check that we are indeed reading the MSF
    // format.
    char magic[32];

    // Page size. Always a power of 2. Usually 4096.
    uint32_t pageSize;

    // Page number of the free page map.
    uint32_t freePageMap;

    // Number of pages. The length of the file should always be equal to the
    // page size multiplied by the page count.
    uint32_t pageCount;

    // Information about the "stream table" stream.
    STREAM_INFO streamTableInfo;

    // An array of pages numbers that constitutes the stream table stream
    // pages list. The stream table stream can potentially take up multiple
    // pages, but it usually only takes up one page. It is also usually the
    // last page in the file.
    //uint32_t streamTablePagesPages[0];
};

// Magic version string in the MSF header.
const char kMsfHeaderMagic[] = "Microsoft C/C++ MSF 7.00\r\n\x1a\x44\x53\0\0";

static_assert(sizeof(MSF_HEADER::magic) == sizeof(kMsfHeaderMagic),
        "Invalid MSF header magic string size");
