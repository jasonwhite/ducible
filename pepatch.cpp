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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "pefile.h"
#include "pemap.h"
#include "pepatch.h"

namespace {

/**
 * A range of memory to patch. This is used to keep track of what needs to be
 * patched in the PE file.
 *
 * All the patch locations need to be found before finishing parsing. If we
 * patched while parsing, then parsing could fail and we could be left with an
 * incomplete patch. Thus, we keep a list of patches and patch everything all at
 * once to mitigate failure cases.
 */
struct Patch
{
    // Location to patch.
    uint8_t* addr;

    // Length of the data.
    size_t length;

    // Data overwrite the given location with.
    const uint8_t* data;

    Patch(uint8_t* addr, size_t length, const uint8_t* data)
        : addr(addr), length(length), data(data)
    {}

    template<typename T>
    Patch(T* addr, const T* data)
        : addr((uint8_t*)addr),
          length(sizeof(T)),
          data((const uint8_t*)data)
    {
    }

    /**
     * Applies the patch. Note that no bounds checking is done. It is assumed
     * that it has already been done.
     */
    void apply() {
        for (size_t i = 0; i < length; ++i)
            *addr = data[i];
    }
};

class Patches
{
private:
    // List of patches
    std::vector<Patch> _patches;

public:

    Patches() {}
    ~Patches() {}

    void add(Patch patch) {
        _patches.push_back(patch);
    }

    void applyAll() {
        for (auto&& patch: _patches)
            patch.apply();
    }
};


}

void patchImage(const char* imagePath, const char* pdbPath) {
    MemMap image(imagePath);

    // Replacement for timestamps
    const uint32_t timestamp = 0;

    uint8_t* pStart = (uint8_t*)image.buf();
    uint8_t* pEnd   = pStart + image.length();
    uint8_t* p      = pStart;

    Patches patches;

    if (image.length() < sizeof(IMAGE_DOS_HEADER))
        throw InvalidImage("missing DOS header");

    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)p;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        throw InvalidImage("invalid DOS signature");

    // Skip to the NT headers. Note that we assume this is a PE32 (not a PE32+)
    // header for now.
    p += dosHeader->e_lfanew;
    if (p + sizeof(IMAGE_NT_HEADERS32) >= pEnd)
        throw InvalidImage("missing IMAGE_NT_HEADERS");

    IMAGE_NT_HEADERS32* ntHeaders = (IMAGE_NT_HEADERS32*)p;

    // Check the signature
    if (ntHeaders->Signature != *(const uint32_t*)"PE\0\0")
        throw InvalidImage("invalid PE signature");

    // Eliminate non-determinism
    patches.add(Patch(&ntHeaders->FileHeader.TimeDateStamp, &timestamp));
    patches.add(Patch(&ntHeaders->OptionalHeader.CheckSum, &timestamp));

    patches.applyAll();
}
