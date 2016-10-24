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

#include "pdb/pdb.h"

/**
 * Reads the name map table in the PDB header stream. This is a map of strings
 * to stream numbers.
 *
 * The format is as follows:
 *
 *  1. String buffer:
 *     (a) stringsLength (4 bytes): The size of the string buffer
 *     (b) strings (stringsLength bytes): A list of null-terminated strings.
 *  2. The map of strings to stream indices:
 *     (a) elemCount (4 byte): The number of items in the map (aka its
 *         cardinality).
 *     (b) elemCountMax (4 bytes): The capacity of the map.
 *     (c) Bitset of present elements. This keeps track of which 'holes' have
 *         been filled in the map. There should be elemCount bits set in this
 *         bitset.
 *         i. count (4 bytes): The number of elements in the bitset
 *         ii. bitset (count * 4 bytes): The bits
 *     (c) Bitmap of deleted elements
 *         i. count (4 bytes): The number of elements in the bitset
 *         ii. bitset (count * 4 bytes): The bits
 *     (d) A list of elemCount (string offset, stream index) pairs.
 *
 * Microsoft's PDB implementation was used as a reference. More specifically,
 * see the following files:
 *
 *  1. PDB/include/nmtni.h - NMTNI::reload() - for loading the name table from
 *     disk, which includes a Map.
 *  2. PDB/include/map.h - Map::reload() - for loading a Map from disk.
 *  3. PDB/include/iset.h - ISet::reload() - for loading a bitset from disk,
 *     which is just an Array of longs.
 */
NameMapTable readNameMapTable(const uint8_t* data, const uint8_t* dataEnd) {
    NameMapTable table;

    // Parse the name map
    if (size_t(dataEnd - data) < sizeof(uint32_t))
        throw InvalidPdb("missing PDB name table strings length");

    const uint32_t stringsLength = *(const uint32_t*)data;
    data += sizeof(stringsLength);

    if (size_t(dataEnd - data) < stringsLength)
        throw InvalidPdb("missing PDB name table strings data");

    // The names of the streams. We'll index into this later.
    const char* strings = (const char*)data;
    data += stringsLength;

    if (size_t(dataEnd - data) < 2 * sizeof(uint32_t))
        throw InvalidPdb("missing PDB stream name map sizes");

    // The number of elements in the hash table.
    const uint32_t elemCount = *(const uint32_t*)data;
    data += sizeof(elemCount);

    // The maximum number of elements in the hash table.
    const uint32_t elemCountMax = *(const uint32_t*)data;
    data += sizeof(elemCountMax);

    if (size_t(dataEnd - data) < sizeof(uint32_t))
        throw InvalidPdb("missing PDB name table 'present' bitset size");

    // Skip over the "present" bitset.
    const uint32_t presentSize = *(const uint32_t*)data;
    data += sizeof(presentSize);
    data += presentSize * sizeof(uint32_t);

    if (data > dataEnd)
        throw InvalidPdb("missing PDB name table 'present' bitset data");

    if (size_t(dataEnd - data) < sizeof(uint32_t))
        throw InvalidPdb("missing PDB name table 'deleted' bitset size");

    // Skip over the "deleted" bitset.
    const uint32_t deletedSize = *(const uint32_t*)data;
    data += sizeof(deletedSize);
    data += deletedSize * sizeof(uint32_t);

    if (data > dataEnd)
        throw InvalidPdb("missing PDB name table 'deleted' bitset data");

    if (size_t(dataEnd - data) < elemCount * sizeof(uint32_t) * 2)
        throw InvalidPdb("missing PDB name table pairs");

    // Finally, read the pairs of string offsets and stream indices
    const uint32_t* pairs = (const uint32_t*)data;
    for (size_t i = 0; i < elemCount; ++i) {
        const uint32_t offset = pairs[i*2];

        if (offset >= stringsLength)
            throw InvalidPdb("invalid PDB name table offset into strings buffer");

        const char* name = &strings[offset];
        const uint32_t stream = pairs[i*2+1];
        table[std::string(name)] = stream;
    }

    return table;
}
