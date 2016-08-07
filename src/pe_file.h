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

#include <stdint.h>
#include <stdlib.h>

#include "pe_format.h"

/**
 * Thrown when an image is found to be invalid.
 */
class InvalidImage
{
private:
    const char* _why;

public:

    InvalidImage(const char* why) : _why(why) {}

    const char* why() const {
        return _why;
    }
};

/**
 * Helper class for parsing image headers.
 */
class PEFile
{
private:

    // Pointer to the optional header.
    const uint8_t* _optional;

    void _init();

public:

    const uint8_t* buf;
    const size_t length;

    const IMAGE_DOS_HEADER* dosHeader;
    const IMAGE_FILE_HEADER* fileHeader;
    const IMAGE_SECTION_HEADER* sectionHeaders;

    // Replacement for timestamps
    //
    // The timestamp can't just be set to zero as that represents a special
    // value in the PE file. We set it to some arbitrary fixed date in the past.
    // This is Jan 1, 2010, 0:00:00 GMT. This date shouldn't be too far in the
    // past, otherwise Windows might trigger a warning saying that the
    // instrumented image has known incompatibility issues when someone tries to
    // run it.
    const uint32_t timestamp = 1262304000;

    // Replacement for the PDB age. Starting at 1, this is normally incremented
    // every time the PDB file is incrementally updated. However, for our
    // purposes, we want to keep this at 1.
    const uint32_t pdbAge = 1;

    // Replacement for the PDB GUID. This is calculated by taking the MD5
    // checksum of the PE file skipping over the parts that we patch. Thus, we
    // can mark the PDB signature to be patched with these bytes, but calculate
    // the signature before actually applying the patch.
    uint8_t pdbSignature[16];

    PEFile(const uint8_t* buf, size_t length);

    /**
     * The Magic field of the optional header. This is used to determine if the
     * optional header is 32- or 64-bit.
     */
    uint16_t magic() const {
        return *(uint16_t*)_optional;
    }

    /**
     * Returns the optional header of the given type.
     */
    template<typename T>
    const T* optional() const {
        // Bounds check
        if (_optional + sizeof(T) >= buf + length)
            throw InvalidImage("missing IMAGE_OPTIONAL_HEADER");

        return (T*)_optional;
    }

    /**
     * Translates a relative virtual address (RVA) to a physical address within
     * the mapped file. Note that this does not do any bounds checking. You must
     * check the bounds before dereferencing the returned pointer.
     */
    const uint8_t* translate(size_t rva) const;

    /**
     * Checks if the given pointer for type T is contained in the image. That
     * is, if the object it points to fits inside as well.
     */
    template<typename T>
    bool isValidReference(const T* p) const {
        return ((const uint8_t*)p >= buf) &&
               ((const uint8_t*)p + sizeof(T) <= buf + length);
    }
};
