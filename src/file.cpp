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

#include "file.h"
#include <iostream>

template<> const FileMode<char> FileMode<char>::readExisting("rb");
template<> const FileMode<char> FileMode<char>::writeEmpty("wb");

template<> const FileMode<wchar_t> FileMode<wchar_t>::readExisting(L"rb");
template<> const FileMode<wchar_t> FileMode<wchar_t>::writeEmpty(L"wb");

/**
 * Deletion object to be used with shared_ptr.
 */
class FileCloser {
public:
    void operator()(FILE* f) const {
        fclose(f);
    }
};

#ifdef _WIN32

FileRef openFile(const char* path, FileMode<char> mode) {
    FILE* f = NULL;
    fopen_s(&f, path, mode.mode);
    return FileRef(f, FileCloser());
}

FileRef openFile(const wchar_t* path, FileMode<wchar_t> mode) {
    FILE* f = NULL;
    _wfopen_s(&f, path, mode.mode);
    return FileRef(f, FileCloser());
}

#else // !_WIN32

FileRef openFile(const char* path, FileMode<char> mode) {
    return FileRef(fopen(path, mode.mode), FileCloser());
}

#endif // _WIN32
