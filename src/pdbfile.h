/**
 * Declarations of data structures in PDB files.
 *
 * These are, for the most part, undocumented. At the time of this writing there
 * is no detailed documentation on the PDB file format. Microsoft has open
 * sourced the PDB file format:
 *
 *     https://github.com/Microsoft/microsoft-pdb
 *
 * This is among the worst professional code I've ever seen. It may as well be
 * obfuscated. There is no documentation and the code is horribly crufty and
 * difficult to read. It is a wonder how anyone at Microsoft is able to maintain
 * that obfuscated mess.
 *
 * Since Microsoft hasn't done it, I'll do my best to document the format here
 * as I decrypt it.
 *
 * Common terminology in the microsoft-pdb repository:
 *
 *  - PN  = Page number
 *  - UPN = Universal page number
 *  - CB  = Count of bytes
 *  - FPM = Free page map
 */
#pragma once


/**
 * The PDB file starts with the MultiStream File (MSF) header. There are two
 * types of MSF headers:
 *
 *  1. The PDB 2.0 format.
 *  2. The MSF 7.0 format.
 *
 * These are distinguished by a magic string, but we only care about the second
 * format. Thus, we will ignore the existence of the first because it is a very
 * old format.
 */

const size_t kMaxDirPages = 73;
const size_t kMaxPageSize = 0x1000;
const char kMsfHeaderMagic[] = "Microsoft C/C++ MSF 7.00\r\n\x1a\x44\x53\0\0";

struct MsfHeader {
    char magic[32];       // version string
    uint32_t pageSize;    // page size
    uint32_t freePageMap; // page index of valid FPM
    uint32_t pageCount;   // Number of pages
    uint32_t directorySize; // Size of the directory in bytes
    uint32_t reserved;
    uint32_t pages[kMaxDirPages];
};
