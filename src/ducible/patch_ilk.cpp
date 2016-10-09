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

#include <string>
#include <string.h>
#include <memory>
#include <algorithm>
#include <iostream>

#include "ducible/patch_ilk.h"

#include "util/memmap.h"

// Helpers for CharT generalization
template<typename CharT>
constexpr CharT ilkExtension[] = {};

template<> constexpr char ilkExtension<char>[] = ".ilk";
template<> constexpr wchar_t ilkExtension<wchar_t>[] = L".ilk";

template<typename CharT>
void patchIlkImpl(const CharT* imagePath, const uint8_t oldSignature[16],
        const uint8_t newSignature[16], bool dryrun) {

    std::basic_string<CharT> ilkPath(imagePath);
    size_t extpos = ilkPath.find_last_of('.');

    // Strip off the extension.
    if (extpos != std::basic_string<CharT>::npos)
        ilkPath.resize(extpos);

    ilkPath.append(ilkExtension<CharT>);

    // Map the ilk file into memory.
    try {
        MemMap ilk(ilkPath.c_str());

        uint8_t* buf = (uint8_t*)ilk.buf();
        uint8_t* bufEnd = buf + ilk.length();

        // Find
        uint8_t* it = std::find_first_of(buf, bufEnd,
                oldSignature, oldSignature+16);

        // Replace
        if (it != bufEnd) {
            std::cout << "Replacing old PDB signature in ILK file.\n";

            if (!dryrun)
                memcpy(it, newSignature, 16);
        }
    }
    catch (const std::system_error&) {
        // Ignore.
    }
}

#if defined(_WIN32) && defined(UNICODE)

void patchIlk(const wchar_t* imagePath, const uint8_t oldSignature[16],
        const uint8_t newSignature[16], bool dryrun) {
    patchIlkImpl(imagePath, oldSignature, newSignature, dryrun);
}

#else

void patchIlk(const char* imagePath, const uint8_t oldSignature[16],
        const uint8_t newSignature[16], bool dryrun) {
    patchIlkImpl(imagePath, oldSignature, newSignature, dryrun);
}

#endif
