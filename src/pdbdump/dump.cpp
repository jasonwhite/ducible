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

#include <iostream>

#include "pdbdump/dump.h"

#include "util/file.h"
#include "msf/msf.h"
#include "msf/stream.h"

namespace {

/**
 * Prints the stream table.
 */
void dumpStreamTable(MsfFile& msf) {
    const size_t streamCount = msf.streamCount();

    std::cout << "Streams (" << streamCount << "):\n";

    for (size_t i = 0; i < streamCount; ++i) {
        auto stream = msf.getStream(i);
        std::cout << "  " << i
            << " (" << stream->length() << " bytes, "
            << pageCount<size_t>(4096, stream->length()) << " pages)"
            << std::endl;
    }
}

void dumpPdb(MsfFile& msf) {
    dumpStreamTable(msf);
}

template<typename CharT>
void dumpPdbImpl(const CharT* path) {
    auto pdb = openFile(path, FileMode<CharT>::readExisting);

    MsfFile msf(pdb);

    dumpPdb(msf);
}

}

#if defined(_WIN32) && defined(UNICODE)

void dumpPdb(const wchar_t* path) {
    dumpPdbImpl(path);
}

#else

void dumpPdb(const char* path) {
    dumpPdbImpl(path);
}

#endif
