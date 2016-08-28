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

#include <cstdio>
#include <memory>

/**
 * Abstracts file mode so we can use them generically with other templates.
 */
template<typename CharT>
struct FileMode {
    const CharT* mode;

    FileMode(const CharT* mode) : mode(mode) {}

    static const FileMode<CharT> readExisting;
    static const FileMode<CharT> writeEmpty;
};


typedef std::shared_ptr<FILE> FileRef;

/**
 * Helper functions for opening a file generically and with reference counting.
 */
FileRef openFile(const char* path, FileMode<char> mode = FileMode<char>::readExisting);

/*
 * Renames a file in a platform independent way.
 *
 * Throws std::system_error if it failed.
 */
void renameFile(const char* src, const char* dest);

/*
 * Deletes a file in a platform independent way.
 *
 * Throws std::system_error if it failed.
 */
void deleteFile(const char* path);

#ifdef _WIN32

FileRef openFile(const wchar_t* path, FileMode<wchar_t> mode = FileMode<wchar_t>::readExisting);

void renameFile(const wchar_t* src, const wchar_t* dest);
void deleteFile(const wchar_t* path);

#endif // _WIN32
