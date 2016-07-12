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
#include <iostream>
#include <iomanip>

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
    size_t offset;

    // Length of the data.
    size_t length;

    // Data overwrite the given location with.
    const uint8_t* data;

    // Name of the patch. Useful to see what's going on.
    const char* name;

    Patch(size_t offset, size_t length, const uint8_t* data, const char* name = NULL)
        : offset(offset),
          length(length),
          data(data),
          name(name)
    {}

    template<typename T>
    Patch(size_t offset, const T* data, const char* name = NULL)
        : offset(offset),
          length(sizeof(T)),
          data((const uint8_t*)data),
          name(name)
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const Patch& patch);

    /**
     * Applies the patch. Note that no bounds checking is done. It is assumed
     * that it has already been done.
     */
    void apply(uint8_t* buf, size_t bufLength, bool dryRun) {
        std::cout << *this << std::endl;

        if (!dryRun) {
            for (size_t i = 0; i < length; ++i)
                buf[i] = data[i];
        }
    }
};

std::ostream& operator<<(std::ostream& os, const Patch& patch) {
    os << "Patching '" << patch.name
       << "' at 0x" << std::hex << patch.offset << std::dec
       << " (" << patch.length << " bytes)";
    return os;
}

class Patches
{
private:
    // List of patches
    std::vector<Patch> _patches;

    uint8_t* _buf;
    size_t _length;

public:

    Patches(uint8_t* buf, size_t length) : _buf(buf), _length(length) {}

    void add(Patch patch) {
        _patches.push_back(patch);
    }

    template<typename T>
    void add(T* addr, const T* data, const char* name = NULL)
    {
        add(Patch((uint8_t*)addr - _buf, data, name));
    }

    void applyAll(bool dryRun = false) {
        for (auto&& patch: _patches)
            patch.apply(_buf, _length, dryRun);
    }
};


}

void patchImage(const char* imagePath, const char* pdbPath, bool dryRun) {
    MemMap image(imagePath);

    // Replacement for timestamps
    const uint32_t timestamp = 0;

    size_t i = 0;

    uint8_t* buf = (uint8_t*)image.buf();
    const size_t length = image.length();

    Patches patches(buf, length);

    if (length < sizeof(IMAGE_DOS_HEADER))
        throw InvalidImage("missing DOS header");

    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)(buf+i);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        throw InvalidImage("invalid DOS signature");

    // Skip to the NT headers. Note that we assume this is a PE32 (not a PE32+)
    // header for now.
    i += dosHeader->e_lfanew;
    if (i + sizeof(IMAGE_NT_HEADERS32) >= length)
        throw InvalidImage("missing IMAGE_NT_HEADERS");

    IMAGE_NT_HEADERS32* ntHeaders = (IMAGE_NT_HEADERS32*)(buf+i);

    // Check the signature
    if (ntHeaders->Signature != *(const uint32_t*)"PE\0\0")
        throw InvalidImage("invalid PE signature");

    patches.add(&ntHeaders->FileHeader.TimeDateStamp, &timestamp,
            "FileHeader.TimeDateStamp");
    patches.add(&ntHeaders->OptionalHeader.CheckSum, &timestamp,
            "OptionalHeader.CheckSum");

    patches.applyAll(dryRun);
}
