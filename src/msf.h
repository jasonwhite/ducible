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
#include <stdio.h> // For FILE*
#include <vector>
#include <memory>

#include "msf_format.h"
#include "file.h"

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

typedef std::shared_ptr<MsfStream> MsfStreamRef;

class MsfFile {
private:

    std::vector<MsfStreamRef> _streams;

public:

    MsfFile(FileRef f);

    virtual ~MsfFile();

    /**
     * Adds a new stream and takes ownership of it. Returns the index of the
     * stream.
     */
    size_t addStream(MsfStream* stream);

    /**
     * Returns the stream with the given index. Returns nullptr if it doesn't
     * exist.
     */
    MsfStreamRef getStream(size_t index);

    /**
     * Replaces a stream. Set to NULL to remove it.
     */
    void replaceStream(size_t index, MsfStream* stream);

    /**
     * Returns the number of streams.
     */
    size_t streamCount() const;

    /**
     * Writes this MsfFile out to a new file. A new header, FPM, and stream
     * table will be created.
     *
     * Throws: MsfWriteError if the write fails.
     */
    void write(FileRef f) const;
};
