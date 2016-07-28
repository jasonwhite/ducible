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
    void apply(uint8_t* buf, bool dryRun) {
        std::cout << *this << std::endl;

        if (!dryRun) {
            for (size_t i = 0; i < length; ++i)
                buf[offset+i] = data[i];
        }
    }
};

std::ostream& operator<<(std::ostream& os, const Patch& patch) {
    os << "Patching '" << patch.name
       << "' at offset 0x" << std::hex << patch.offset << std::dec
       << " (" << patch.length << " bytes)";
    return os;
}

class Patches
{
private:
    // List of patches
    std::vector<Patch> _patches;

    uint8_t* _buf;

public:

    Patches(uint8_t* buf) : _buf(buf) {}

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
            patch.apply(_buf, dryRun);
    }
};

/**
 * Patches the timestamp associated with a data directory.
 */
template<typename T>
void patchDataDirectory(uint8_t* buf, const size_t length, Patches& patches,
        const IMAGE_DATA_DIRECTORY* imageDataDirs,
        uint32_t entry, const char* name, uint32_t timestamp) {

    const IMAGE_DATA_DIRECTORY& imageDataDir = imageDataDirs[entry];

    // Doesn't exist? Nothing to patch.
    if (imageDataDir.VirtualAddress == 0)
        return;

    if (imageDataDir.Size < sizeof(T)) {
        // Note that we check if the size is less than our defined struct.
        // Microsoft is free to add elements to the end of the struct in future
        // versions as that still maintains ABI compatibility.
        throw InvalidImage("invalid IMAGE_DATA_DIRECTORY size");
    }

    if (imageDataDir.VirtualAddress + imageDataDir.Size >= length) {
        throw InvalidImage("invalid IMAGE_DATA_DIRECTORY offset");
    }

    T* dir = (T*)(buf + imageDataDir.VirtualAddress);
    if (dir->TimeDateStamp != 0) {
        patches.add(&dir->TimeDateStamp, &timestamp, name);
    }
}

/**
 * Patches the image based on the optional header type. The optional header can
 * be either 32- or 64-bit.
 */
template<typename T>
void patchOptionalHeader(T* header, uint8_t* buf, const size_t length,
        Patches& patches, uint32_t timestamp) {

    patches.add(&header->CheckSum, &timestamp,
            "OptionalHeader.CheckSum");

    const IMAGE_DATA_DIRECTORY* dataDirs = header->DataDirectory;

    // Patch exports directory timestamp
    patchDataDirectory<IMAGE_EXPORT_DIRECTORY>(buf, length, patches,
            dataDirs, IMAGE_DIRECTORY_ENTRY_EXPORT,
            "IMAGE_EXPORT_DIRECTORY.TimeDateStamp", timestamp);

    // Patch resource directory timestamp
    patchDataDirectory<IMAGE_RESOURCE_DIRECTORY>(buf, length, patches,
            dataDirs, IMAGE_DIRECTORY_ENTRY_RESOURCE,
            "IMAGE_RESOURCE_DIRECTORY.TimeDateStamp", timestamp);
}

}

void patchImage(const char* imagePath, const char* pdbPath, bool dryRun) {
    MemMap image(imagePath);
    MemMap pdb(pdbPath);

    // Replacement for timestamps
    const uint32_t timestamp = 0;

    size_t i = 0;

    uint8_t* buf = (uint8_t*)image.buf();
    const size_t length = image.length();

    Patches patches(buf);

    if (length < sizeof(IMAGE_DOS_HEADER))
        throw InvalidImage("missing DOS header");

    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)(buf+i);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        throw InvalidImage("invalid DOS signature");

    // Skip to the NT headers. Note that we don't parse this section as
    // IMAGE_NT_HEADERS32/IMAGE_NT_HEADERS64 because we don't yet know if this
    // image is 32- or 64-bit. That information is in the first field of the
    // optional header.
    i += dosHeader->e_lfanew;

    // Check the signature
    const uint32_t signature = *(uint32_t*)(buf+i);

    if (i + sizeof(signature) >= length)
        throw InvalidImage("missing PE signature");

    if (signature != *(const uint32_t*)"PE\0\0")
        throw InvalidImage("invalid PE signature");

    i += sizeof(signature);

    // Parse the file header
    IMAGE_FILE_HEADER* fileHeader = (IMAGE_FILE_HEADER*)(buf+i);

    if (i + sizeof(*fileHeader) >= length)
        throw InvalidImage("missing IMAGE_FILE_HEADER");

    i += sizeof(*fileHeader);

    patches.add(&fileHeader->TimeDateStamp, &timestamp,
            "IMAGE_FILE_HEADER.TimeDateStamp");

    // This is the Magic field of IMAGE_OPTIONAL_HEADER.
    const uint16_t magic = *(uint16_t*)(buf+i);

    switch (magic) {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
            // Patch as a PE32 file
            patchOptionalHeader((IMAGE_OPTIONAL_HEADER32*)(buf+i), buf, length,
                    patches, timestamp);
            break;

        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
            // Patch as a PE32+ file
            patchOptionalHeader((IMAGE_OPTIONAL_HEADER64*)(buf+i), buf, length,
                    patches, timestamp);
            break;

        default:
            throw InvalidImage("unsupported IMAGE_NT_HEADERS.OptionalHeader");
    }

    patches.applyAll(dryRun);
}
