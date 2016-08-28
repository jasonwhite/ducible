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

#include <stdlib.h>
#include <stdint.h>

/**
 * PDB stream IDs.
 */
namespace PdbStreamType {
    enum {

        // Stream table stream.
        streamTable = 0,

        // Version information, and information to connect this PDB to the EXE.
        header = 1,

        // Type information stream. All the types used in the executable.
        tbi = 2,

        // Debug information stream. Holds section contributions, and list of
        // ‘Mods’.
        dbi = 3,

        // ID info stream. Holds a hashed string table.
        ipi = 4,

        // There are more streams than this, but they are not accessed directly
        // by a stream ID constant. We are usually only interested in the above
        // streams anyway.
    };
}

/**
 * Implementation version of the PDB.
 */
namespace PdbVersion {
    enum {
        vc2     = 19941610,
        vc4     = 19950623,
        vc41    = 19950814,
        vc50    = 19960307,
        vc98    = 19970604,
        vc70Dep = 19990604, // deprecated vc70 implementation version
        vc70    = 20000404,
        vc80    = 20030901,
        vc110   = 20091201,
        vc140   = 20140508,
    };
}

/**
 * PDB stream.
 */
struct PdbStream {
    // Implementation version number.
    uint32_t version;

    // Timestamp of when the PDB was created.
    uint32_t timestamp;

    // Number of times this PDB instance has been updated.
    uint32_t age;
};

static_assert(sizeof(PdbStream) == 12);

/**
 * PDB 7.0 stream.
 */
struct PdbStream70 : public PdbStream {
    // PDB GUID. This must match the PE file.
    uint8_t sig70[16];
};

static_assert(sizeof(PdbStream70) == 28);

/**
 * The DBI header signature.
 */
const uint32_t dbiHeaderSignature = -1;

/**
 * The DBI implementation version.
 */
namespace DbiVersion {
    enum {
        v41  = 930803,
        v50  = 19960307,
        v60  = 19970606,
        v70  = 19990903,
        v110 = 20091201,
    };
}

/**
 * The Debug Information Stream (DBI) header.
 */
struct DbiHeader {

    // The header signature. This should be 0xffffffff.
    uint32_t signature;

    // The header version
    uint32_t version;
    uint32_t age;

    // Stream number of the global symbols
    uint16_t globalSymbolsStream;

    // PDB DLL version
    struct PdbDllVersion {
        uint16_t minor : 8;
        uint16_t major : 7;
        uint16_t format : 1;
    } pdbDllVersion;

    // Stream number of the public symbols
    uint16_t publicSymbolsStream;

    uint16_t pdbDllBuildVersionMajor;

    // Stream number of the symbol records
    uint16_t symbolRecordsStream;

    uint16_t pdbDllBuildVersionMinor;

    // Size of the module info substream
    uint32_t gpModInfoSize;

    // Size of the section contribution substream
    uint32_t sectionContributionSize;

    // Size of the section map substream
    uint32_t sectionMapSize;

    // Size of the file info substream
    uint32_t fileInfoSize;

    // Size of the type server substream
    uint32_t typeServerMapSize;

    // Index of the MFC type server
    uint32_t mfcIndex;

    // Size of the optional debug header that is appended to the end of the
    // stream.
    uint32_t debugHeaderSize;

    // Size of the EC info substream.
    uint32_t ecInfoSize;

    struct Flags {
        uint16_t incLink : 1;  // True if linked incrementally (really just if
                               // ilink thunks are present)
        uint16_t stripped : 1; // True if private data is stripped out
        uint16_t ctypes : 1;   // True if this PDB is using CTypes.
        uint16_t unused : 13;  // reserved, must be 0.
    } flags;

    // Machine type
    uint16_t machine;

    // Padded to 64 bytes for future growth
    uint32_t reserved[1];
};

static_assert(sizeof(DbiHeader) == 64);

/**
 * Section contribution.
 */
struct SectionContribution {
    // Section index
    uint16_t section;

    // Padding due to struct alignment in Microsoft's implementation. They do
    // not zero this and thus may contain garbage when serialized to the PDB
    // file.
    uint16_t padding1;

    int32_t offset;
    uint32_t size;
    uint32_t characteristics;
    uint16_t imod;

    // Padding due to struct alignment in Microsoft's implementation. They do
    // not zero this and thus may contain garbage when serialized to the PDB
    // file.
    uint16_t padding2;

    uint32_t dataCrc;
    uint32_t relocCrc;
};

static_assert(sizeof(SectionContribution) == 28);

/**
 * Module info.
 */
struct ModuleInfo {
    uint32_t opened;

    SectionContribution sc;

    struct Flags {
        uint16_t written : 1;   // True if mod has been written since DBI opened
        uint16_t ecEnabled : 1; // True if mod has EC symbolic information
        uint16_t unused: 6;
        uint16_t tsmIndex: 8;   // index into TSM list for this mods server
    } flags;

    uint16_t stream; // Stream number of module debug info

    uint32_t symbolsSize;  // Size of local symbols debug info in stream
    uint32_t linesSize;    // Size of line number debug info in stream
    uint32_t c13LinesSize; // Size of C13 style line number info in stream

    uint16_t fileCount; // number of files contributing to this module

    uint32_t offsets;

    uint32_t srcFileIndex;
    uint32_t pdbFileIndex;

    // NUL-terminated module name followed by NUL-terminated object file name
    char names[];

    // After the names, the struct is aligned to 4-bytes.

    /**
     * Returns the module name.
     */
    const char* moduleName() const {
        return names;
    }

    /**
     * Returns the object name.m
     */
    const char* objectName() const {
        const char* name = moduleName();

        while (*name != 0) ++name;
        ++name;

        return name;
    }

    /*
     * Returns the total size of this struct, including the names, and
     * padding due to alignment.
     */
    size_t size() const {

        size_t len = sizeof(*this);

        // Skip past the names
        const char* p = names;
        while (*p != 0) { ++p; } ++p; // Skip module name
        while (*p != 0) { ++p; } ++p; // Skip object name
        len += p - names;

        // Align to a multiple of 4 bytes
        return (len + 3) & -4;
    }
};

static_assert(sizeof(ModuleInfo) == 64);

/**
 * Thrown when a PDB is found to be invalid or unsupported.
 */
class InvalidPdb
{
private:
    const char* _why;

public:

    InvalidPdb(const char* why) : _why(why) {}

    const char* why() const {
        return _why;
    }
};
