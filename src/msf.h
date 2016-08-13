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

/**
 * MultiStream File (MSF) Format Overview:
 *
 * At a high level, the MSF format is used to implement *streams*. A stream
 * consists of one or more *pages*. Each stream can be thought of as a
 * standalone file embedded in a regular file. The pages in a stream are not
 * necessarily sequential in the file; they can be located anywhere in the file
 * and in any order.
 *
 * PDBs are implemented in this format for a couple of reasons:
 *
 *  1. The developers of the PDB format wanted a single file on disk for debug
 *     information instead of having a multitude of similar files on disk. This
 *     also simplifies things for the user.
 *  2. Changes to the PDB file can be atomically committed just as with a
 *     database. This simplifies things for the compiler/linker when updating
 *     the PDB file.
 *
 * The first page in the MSF is special. It contains the MSF header
 * (`MSF_HEADER`) and the list of pages that comprise the stream table. The MSF
 * header is documented below, but the stream table needs further explanation.
 *
 * The stream table lists the different streams and the pages that constitute
 * each stream. The stream table itself is also a stream. Thus, in order to read
 * the stream table stream, we need to know the page numbers for the stream
 * table. This information is in the MSF header.
 */

#include <stdint.h>
#include <stdio.h> // For FILE*
#include <vector>
#include <memory>

#include "msf_format.h"

/**
 * Thrown when an MSF is found to be invalid or unsupported.
 */
class InvalidMsf
{
private:
    const char* _why;

public:

    InvalidMsf(const char* why) : _why(why) {}

    const char* why() const {
        return _why;
    }
};

class MsfStream;

class MsfFile {
private:

    MSF_HEADER _header;
    std::vector<std::shared_ptr<MsfStream> > _streams;

    void writeHeader(FILE* f);

public:

    MsfFile(FILE* f);

    /**
     * Returns the page size.
     */
    uint32_t pageSize() const;

    /**
     * Returns the number of pages.
     */
    uint32_t pageCount() const;

    /**
     * Adds a new stream. Returns the index of the stream.
     */
    size_t addStream(MsfStream* stream);

    /**
     * Returns the stream with the given index. Returns nullptr if it doesn't
     * exist.
     */
    std::shared_ptr<MsfStream> getStream(size_t index);

    /**
     * Replaces a stream. Set to NULL to remove it.
     */
    void replaceStream(size_t index, MsfStream* stream);

    /**
     * Returns the number of streams.
     */
    size_t streamCount() const;

    /**
     * Writes this MsfFile out to a new file.
     */
    void write(FILE* f) const;
};
