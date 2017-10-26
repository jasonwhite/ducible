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

/**
 * This file maps a PE file into memory in a cross-platform way. Using a memory
 * map is much more efficient than loading the entire file into memory and then
 * writing it back out. If loading a PE file with very large embedded resources,
 * it would otherwise be quite slow. Rather, with a memory map, the operating
 * system handles loading pages into memory only when they are needed.
 */

#pragma once

#include <stdlib.h>  // For size_t

#ifdef _WIN32
typedef void* HANDLE;
#endif

/**
 * Maps a file into memory.
 */
class MemMap {
   private:
    void* _buf;
    size_t _length;

#ifdef _WIN32
    HANDLE _fileMap;
    void _init(HANDLE hFile, size_t length = 0);
#endif

   public:
    MemMap(const char* path, size_t length = 0);
    ~MemMap();

#ifdef _WIN32
    MemMap(const wchar_t* path, size_t length = 0);
#endif

    /**
     * Returns the size of the file.
     */
    size_t length() const { return _length; }

    /**
     * Returns a pointer to the buffer.
     */
    void* buf() { return _buf; }
};
