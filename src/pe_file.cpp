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

#include "pe_file.h"

#include <cstring>

PEFile::PEFile(const uint8_t* buf, size_t length)
    : buf(buf), length(length)
{
    memset(pdbSignature, 0, sizeof(pdbSignature)/sizeof(*pdbSignature));

    _init();
}

void PEFile::_init() {

    const uint8_t* p = buf;
    const uint8_t* end = buf + length;

    if (p + sizeof(IMAGE_DOS_HEADER) >= end)
        throw InvalidImage("missing DOS header");

    dosHeader = (IMAGE_DOS_HEADER*)p;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        throw InvalidImage("invalid DOS signature");

    // Skip to the NT headers. Note that we don't parse this section as
    // IMAGE_NT_HEADERS32/IMAGE_NT_HEADERS64 because we don't yet know if this
    // image is 32- or 64-bit. That information is in the first field of the
    // optional header.
    p += dosHeader->e_lfanew;

    //
    // Check the signature
    //
    if (p + sizeof(uint32_t) >= end)
        throw InvalidImage("missing PE signature");

    const uint32_t signature = *(uint32_t*)p;
    if (signature != *(const uint32_t*)"PE\0\0")
        throw InvalidImage("invalid PE signature");

    p += sizeof(signature);

    //
    // Parse the image file header
    //
    if (p + sizeof(IMAGE_FILE_HEADER) >= end)
        throw InvalidImage("missing IMAGE_FILE_HEADER");

    fileHeader = (IMAGE_FILE_HEADER*)p;

    p += sizeof(IMAGE_FILE_HEADER);

    //
    // The optional header is here. Parsing of this is delayed because it can
    // either be a 32- or 64-bit structure.
    //
    _optional = p;

    p += fileHeader->SizeOfOptionalHeader;

    //
    // Section headers. There are IMAGE_FILE_HEADER.NumberOfSections of these.
    //
    sectionHeaders = (IMAGE_SECTION_HEADER*)p;
}

const uint8_t* PEFile::translate(size_t rva) const {

    const IMAGE_SECTION_HEADER* s = sectionHeaders;

    for (size_t i = 0; i < fileHeader->NumberOfSections; ++i) {
        if (rva >= s->VirtualAddress &&
            rva < s->VirtualAddress + s->Misc.VirtualSize)
            break;

        ++s;
    }

    return buf + rva - s->VirtualAddress + s->PointerToRawData;
}
