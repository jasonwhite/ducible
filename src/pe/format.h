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
 * This header file defines the PE file format structs.
 *
 * These are already defined in <windows.h>, but redefining them lets us build
 * (and use) this tool on platforms other than Windows.
 */

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// Both VC and GCC should support this compiler directive.
#pragma pack(push, 4)

//
// Image Format
//

#define IMAGE_DOS_SIGNATURE 0x5A4D     // MZ
#define IMAGE_OS2_SIGNATURE 0x454E     // NE
#define IMAGE_OS2_SIGNATURE_LE 0x454C  // LE
#define IMAGE_VXD_SIGNATURE 0x454C     // LE
#define IMAGE_NT_SIGNATURE 0x00004550  // PE00

#pragma pack(push, 2)  // 16 bit headers are 2 byte packed

typedef struct _IMAGE_DOS_HEADER {  // DOS .EXE header
    uint16_t e_magic;               // Magic number
    uint16_t e_cblp;                // Bytes on last page of file
    uint16_t e_cp;                  // Pages in file
    uint16_t e_crlc;                // Relocations
    uint16_t e_cparhdr;             // Size of header in paragraphs
    uint16_t e_minalloc;            // Minimum extra paragraphs needed
    uint16_t e_maxalloc;            // Maximum extra paragraphs needed
    uint16_t e_ss;                  // Initial (relative) SS value
    uint16_t e_sp;                  // Initial SP value
    uint16_t e_csum;                // Checksum
    uint16_t e_ip;                  // Initial IP value
    uint16_t e_cs;                  // Initial (relative) CS value
    uint16_t e_lfarlc;              // File address of relocation table
    uint16_t e_ovno;                // Overlay number
    uint16_t e_res[4];              // Reserved words
    uint16_t e_oemid;               // OEM identifier (for e_oeminfo)
    uint16_t e_oeminfo;             // OEM information; e_oemid specific
    uint16_t e_res2[10];            // Reserved words
    int32_t e_lfanew;               // File address of new exe header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

#pragma pack(pop)

//
// File header format.
//

typedef struct _IMAGE_FILE_HEADER {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

#define IMAGE_SIZEOF_FILE_HEADER 20

#define IMAGE_FILE_RELOCS_STRIPPED \
    0x0001  // Relocation info stripped from file.
#define IMAGE_FILE_EXECUTABLE_IMAGE \
    0x0002  // File is executable  (i.e. no unresolved external references).
#define IMAGE_FILE_LINE_NUMS_STRIPPED \
    0x0004  // Line nunbers stripped from file.
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED \
    0x0008  // Local symbols stripped from file.
#define IMAGE_FILE_AGGRESIVE_WS_TRIM 0x0010    // Aggressively trim working set
#define IMAGE_FILE_LARGE_ADDRESS_AWARE 0x0020  // App can handle >2gb addresses
#define IMAGE_FILE_BYTES_REVERSED_LO \
    0x0080                               // Bytes of machine word are reversed.
#define IMAGE_FILE_32BIT_MACHINE 0x0100  // 32 bit word machine.
#define IMAGE_FILE_DEBUG_STRIPPED \
    0x0200  // Debugging info stripped from file in .DBG file
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP \
    0x0400  // If Image is on removable media, copy and run from the swap file.
#define IMAGE_FILE_NET_RUN_FROM_SWAP \
    0x0800  // If Image is on Net, copy and run from the swap file.
#define IMAGE_FILE_SYSTEM 0x1000  // System File.
#define IMAGE_FILE_DLL 0x2000     // File is a DLL.
#define IMAGE_FILE_UP_SYSTEM_ONLY \
    0x4000  // File should only be run on a UP machine
#define IMAGE_FILE_BYTES_REVERSED_HI \
    0x8000  // Bytes of machine word are reversed.

#define IMAGE_FILE_MACHINE_UNKNOWN 0
#define IMAGE_FILE_MACHINE_I386 0x014c   // Intel 386.
#define IMAGE_FILE_MACHINE_R3000 0x0162  // MIPS little-endian, 0x160 big-endian
#define IMAGE_FILE_MACHINE_R4000 0x0166  // MIPS little-endian
#define IMAGE_FILE_MACHINE_R10000 0x0168     // MIPS little-endian
#define IMAGE_FILE_MACHINE_WCEMIPSV2 0x0169  // MIPS little-endian WCE v2
#define IMAGE_FILE_MACHINE_ALPHA 0x0184      // Alpha_AXP
#define IMAGE_FILE_MACHINE_SH3 0x01a2        // SH3 little-endian
#define IMAGE_FILE_MACHINE_SH3DSP 0x01a3
#define IMAGE_FILE_MACHINE_SH3E 0x01a4   // SH3E little-endian
#define IMAGE_FILE_MACHINE_SH4 0x01a6    // SH4 little-endian
#define IMAGE_FILE_MACHINE_SH5 0x01a8    // SH5
#define IMAGE_FILE_MACHINE_ARM 0x01c0    // ARM Little-Endian
#define IMAGE_FILE_MACHINE_THUMB 0x01c2  // ARM Thumb/Thumb-2 Little-Endian
#define IMAGE_FILE_MACHINE_ARMNT 0x01c4  // ARM Thumb-2 Little-Endian
#define IMAGE_FILE_MACHINE_AM33 0x01d3
#define IMAGE_FILE_MACHINE_POWERPC 0x01F0  // IBM PowerPC Little-Endian
#define IMAGE_FILE_MACHINE_POWERPCFP 0x01f1
#define IMAGE_FILE_MACHINE_IA64 0x0200       // Intel 64
#define IMAGE_FILE_MACHINE_MIPS16 0x0266     // MIPS
#define IMAGE_FILE_MACHINE_ALPHA64 0x0284    // ALPHA64
#define IMAGE_FILE_MACHINE_MIPSFPU 0x0366    // MIPS
#define IMAGE_FILE_MACHINE_MIPSFPU16 0x0466  // MIPS
#define IMAGE_FILE_MACHINE_AXP64 IMAGE_FILE_MACHINE_ALPHA64
#define IMAGE_FILE_MACHINE_TRICORE 0x0520  // Infineon
#define IMAGE_FILE_MACHINE_CEF 0x0CEF
#define IMAGE_FILE_MACHINE_EBC 0x0EBC    // EFI Byte Code
#define IMAGE_FILE_MACHINE_AMD64 0x8664  // AMD64 (K8)
#define IMAGE_FILE_MACHINE_M32R 0x9041   // M32R little-endian
#define IMAGE_FILE_MACHINE_CEE 0xC0EE

//
// Directory format.
//

typedef struct _IMAGE_DATA_DIRECTORY {
    uint32_t VirtualAddress;
    uint32_t Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

//
// Optional header format.
//

typedef struct _IMAGE_OPTIONAL_HEADER {
    //
    // Standard fields.
    //

    uint16_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint32_t BaseOfData;

    //
    // NT additional fields.
    //

    uint32_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint32_t SizeOfStackReserve;
    uint32_t SizeOfStackCommit;
    uint32_t SizeOfHeapReserve;
    uint32_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_ROM_OPTIONAL_HEADER {
    uint16_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint32_t BaseOfData;
    uint32_t BaseOfBss;
    uint32_t GprMask;
    uint32_t CprMask[4];
    uint32_t GpValue;
} IMAGE_ROM_OPTIONAL_HEADER, *PIMAGE_ROM_OPTIONAL_HEADER;

typedef struct _IMAGE_OPTIONAL_HEADER64 {
    uint16_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20b
#define IMAGE_ROM_OPTIONAL_HDR_MAGIC 0x107

typedef struct _IMAGE_NT_HEADERS64 {
    uint32_t Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

typedef struct _IMAGE_NT_HEADERS {
    uint32_t Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

typedef struct _IMAGE_ROM_HEADERS {
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_ROM_OPTIONAL_HEADER OptionalHeader;
} IMAGE_ROM_HEADERS, *PIMAGE_ROM_HEADERS;

// Directory Entries

#define IMAGE_DIRECTORY_ENTRY_EXPORT 0     // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT 1     // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE 2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION 3  // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY 4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC 5  // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG 6      // Debug Directory
//      IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7   // (X86 usage)
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE 7  // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR 8     // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS 9           // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG 10  // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT \
    11                                // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT 12  // Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT 13  // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14  // COM Runtime descriptor

//
// Export Format
//

typedef struct _IMAGE_EXPORT_DIRECTORY {
    uint32_t Characteristics;
    uint32_t TimeDateStamp;
    uint16_t MajorVersion;
    uint16_t MinorVersion;
    uint32_t Name;
    uint32_t Base;
    uint32_t NumberOfFunctions;
    uint32_t NumberOfNames;
    uint32_t AddressOfFunctions;     // RVA from base of image
    uint32_t AddressOfNames;         // RVA from base of image
    uint32_t AddressOfNameOrdinals;  // RVA from base of image
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

//
// Resource Format.
//

//
// Resource directory consists of two counts, following by a variable length
// array of directory entries.  The first count is the number of entries at
// beginning of the array that have actual names associated with each entry.
// The entries are in ascending order, case insensitive strings.  The second
// count is the number of entries that immediately follow the named entries.
// This second count identifies the number of entries that have 16-bit integer
// Ids as their name.  These entries are also sorted in ascending order.
//
// This structure allows fast lookup by either name or number, but for any
// given resource entry only one form of lookup is supported, not both.
// This is consistent with the syntax of the .RC file and the .RES file.
//

typedef struct _IMAGE_RESOURCE_DIRECTORY {
    uint32_t Characteristics;
    uint32_t TimeDateStamp;
    uint16_t MajorVersion;
    uint16_t MinorVersion;
    uint16_t NumberOfNamedEntries;
    uint16_t NumberOfIdEntries;
    //  IMAGE_RESOURCE_DIRECTORY_ENTRY DirectoryEntries[];
} IMAGE_RESOURCE_DIRECTORY, *PIMAGE_RESOURCE_DIRECTORY;

//
// Debug Format
//

typedef struct _IMAGE_DEBUG_DIRECTORY {
    uint32_t Characteristics;
    uint32_t TimeDateStamp;
    uint16_t MajorVersion;
    uint16_t MinorVersion;
    uint32_t Type;
    uint32_t SizeOfData;
    uint32_t AddressOfRawData;
    uint32_t PointerToRawData;
} IMAGE_DEBUG_DIRECTORY, *PIMAGE_DEBUG_DIRECTORY;

#define IMAGE_DEBUG_TYPE_UNKNOWN 0
#define IMAGE_DEBUG_TYPE_COFF 1
#define IMAGE_DEBUG_TYPE_CODEVIEW 2
#define IMAGE_DEBUG_TYPE_FPO 3
#define IMAGE_DEBUG_TYPE_MISC 4
#define IMAGE_DEBUG_TYPE_EXCEPTION 5
#define IMAGE_DEBUG_TYPE_FIXUP 6
#define IMAGE_DEBUG_TYPE_OMAP_TO_SRC 7
#define IMAGE_DEBUG_TYPE_OMAP_FROM_SRC 8
#define IMAGE_DEBUG_TYPE_BORLAND 9
#define IMAGE_DEBUG_TYPE_RESERVED10 10
#define IMAGE_DEBUG_TYPE_CLSID 11

//
// Section header format.
//

#define IMAGE_SIZEOF_SHORT_NAME 8

typedef struct _IMAGE_SECTION_HEADER {
    uint8_t Name[IMAGE_SIZEOF_SHORT_NAME];
    union {
        uint32_t PhysicalAddress;
        uint32_t VirtualSize;
    } Misc;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define IMAGE_SIZEOF_SECTION_HEADER 40

//
// CodeView Info
//
// Reference: http://www.debuginfo.com/articles/debuginfomatch.html
//

// This reads 'RSDS' in memory.
#define CV_INFO_SIGNATURE_PDB70 0x53445352

typedef struct {
    // CodeView signature, equal to “RSDS”
    uint32_t CvSignature;

    // A unique identifier, which changes with every rebuild of the executable
    // and PDB file.
    uint8_t Signature[16];

    // Ever-incrementing value, which is initially set to 1 and incremented
    // every time when a part of the PDB file is updated without rewriting the
    // whole file.
    uint32_t Age;

    // Null-terminated name of the PDB file. It can also contain full or partial
    // path to the file.
    char PdbFileName[1];
} CV_INFO_PDB70;

#pragma pack(pop)

#ifdef __cplusplus
} /* extern "C" */
#endif
