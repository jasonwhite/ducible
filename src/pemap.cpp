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

#include "pemap.h"

#if defined(_WIN32)

#include <windows.h>
#include <system_error>

MemMap::MemMap(const char* path, size_t length)
    : _buf(0), _length(0) {
    // TODO: Implement me
}

MemMap::~MemMap() {
}

#else

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <system_error>

MemMap::MemMap(const char* path, size_t length)
    : _buf(0), _length(0) {

    int fd = open(path, O_RDWR);
    if (fd == -1) {
        throw std::system_error(errno, std::system_category(),
                "Failed to open file");
    }

    if (length == 0) {
        struct stat stbuf;
        if (fstat(fd, &stbuf) == -1) {
            throw std::system_error(errno, std::system_category(),
                    "Failed to stat file");
        }

        length = stbuf.st_size;
    }

    void* p = mmap(
            NULL,                   // Preferred base address (don't care)
            length,                 // Length of the memory map
            PROT_READ | PROT_WRITE, // Protection flags
            MAP_SHARED,
            fd,                     // File descriptor
            0                       // Offset within the file
            );

    if (p == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(),
                "Failed to map file");
    }

    _buf = p;
    _length = length;

    // We don't need this open in order to keep the file mapped.
    if (close(fd) == -1) {
        throw std::system_error(errno, std::system_category(),
                "Failed to close file");
    }
}

MemMap::~MemMap() {
    if (_buf) {
        munmap(_buf, _length);
    }
}

#endif
