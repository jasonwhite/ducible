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
#include <codecvt>
#include <string>
#include <sstream>

#ifdef _WIN32
#   include <Windows.h>
#endif

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

    if (fopen_s(&f, path, mode.mode) != 0) {
        auto err = errno;

        std::stringbuf buf;
        std::ostream msg(&buf);

        msg << "Failed to open file '" << path << "'";

        throw std::system_error(err, std::system_category(), buf.str());
    }

    return FileRef(f, FileCloser());
}

FileRef openFile(const wchar_t* path, FileMode<wchar_t> mode) {
    FILE* f = NULL;

    if (_wfopen_s(&f, path, mode.mode) != 0) {
        auto err = errno;

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

        std::stringbuf buf;
        std::ostream msg(&buf);

        msg << "Failed to open file '" << converter.to_bytes(path) << "'";

        throw std::system_error(err, std::system_category(), buf.str());
    }

    return FileRef(f, FileCloser());
}

void renameFile(const char* src, const char* dest) {
    if (!MoveFileExA(src, dest, MOVEFILE_REPLACE_EXISTING)) {
        throw std::system_error(GetLastError(), std::system_category(),
            "failed to rename file");
    }
}

void renameFile(const wchar_t* src, const wchar_t* dest) {
    if (!MoveFileExW(src, dest, MOVEFILE_REPLACE_EXISTING)) {
        auto err = GetLastError();

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

        std::stringbuf buf;
        std::ostream msg(&buf);

        msg << "failed to rename file '" << converter.to_bytes(src)
            << "' to '" << converter.to_bytes(dest) << "'";

        throw std::system_error(err, std::system_category(), buf.str());
    }
}

void deleteFile(const char* path) {
    if (!DeleteFileA(path)) {
        auto err = GetLastError();

        std::stringbuf buf;
        std::ostream msg(&buf);

        msg << "failed to delete file '" << path << "'";

        throw std::system_error(err, std::system_category(), buf.str());
    }
}

void deleteFile(const wchar_t* path) {
    if (!DeleteFileW(path)) {
        auto err = GetLastError();

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

        std::stringbuf buf;
        std::ostream msg(&buf);

        msg << "failed to delete file '" << converter.to_bytes(path) << "'";

        throw std::system_error(err, std::system_category(), buf.str());
    }
}

#else // !_WIN32

FileRef openFile(const char* path, FileMode<char> mode) {
    FILE* f = fopen(path, mode.mode);

    if (!f) {
        auto err = errno;

        std::stringbuf buf;
        std::ostream msg(&buf);

        msg << "Failed to open file '" << path << "'";

        throw std::system_error(err, std::system_category(), buf.str());
    }

    return FileRef(f, FileCloser());
}

void renameFile(const char* src, const char* dest) {
    if (rename(src, dest) != 0) {
        auto err = errno;

        std::stringbuf buf;
        std::ostream msg(&buf);

        msg << "failed to rename file '" << src << "' to '" << dest << "'";

        throw std::system_error(err, std::system_category(), buf.str());
    }
}

void deleteFile(const char* path) {
    if (remove(path) != 0) {
        auto err = errno;

        std::stringbuf buf;
        std::ostream msg(&buf);

        msg << "failed to delete file '" << path << "'";

        throw std::system_error(err, std::system_category(), buf.str());
    }
}

#endif // _WIN32
