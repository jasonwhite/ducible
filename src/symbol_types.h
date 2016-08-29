/*
 * Copyright (c) 2015 Microsoft Corporation. All rights reserved.
 *
 * This code is licensed under the MIT License (MIT).
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && defined(__cplusplus)
#   pragma warning(push)

    // Disable "nonstandard extension used: zero-sized array in struct/union"
#   pragma warning(disable : 4200)
#endif

/**
 * Symbol record types. These correspond to the |type| field in a symbol record.
 */
typedef enum SYM_ENUM_e {
    S_COMPILE       =  0x0001,  // Compile flags symbol
    S_REGISTER_16t  =  0x0002,  // Register variable
    S_CONSTANT_16t  =  0x0003,  // constant symbol
    S_UDT_16t       =  0x0004,  // User defined type
    S_SSEARCH       =  0x0005,  // Start Search
    S_END           =  0x0006,  // Block, procedure, "with" or thunk end
    S_SKIP          =  0x0007,  // Reserve symbol space in $$Symbols table
    S_CVRESERVE     =  0x0008,  // Reserved symbol for CV internal use
    S_OBJNAME_ST    =  0x0009,  // path to object file name
    S_ENDARG        =  0x000a,  // end of argument/return list
    S_COBOLUDT_16t  =  0x000b,  // special UDT for cobol that does not symbol pack
    S_MANYREG_16t   =  0x000c,  // multiple register variable
    S_RETURN        =  0x000d,  // return description symbol
    S_ENTRYTHIS     =  0x000e,  // description of this pointer on entry

    S_BPREL16       =  0x0100,  // BP-relative
    S_LDATA16       =  0x0101,  // Module-local symbol
    S_GDATA16       =  0x0102,  // Global data symbol
    S_PUB16         =  0x0103,  // a public symbol
    S_LPROC16       =  0x0104,  // Local procedure start
    S_GPROC16       =  0x0105,  // Global procedure start
    S_THUNK16       =  0x0106,  // Thunk Start
    S_BLOCK16       =  0x0107,  // block start
    S_WITH16        =  0x0108,  // with start
    S_LABEL16       =  0x0109,  // code label
    S_CEXMODEL16    =  0x010a,  // change execution model
    S_VFTABLE16     =  0x010b,  // address of virtual function table
    S_REGREL16      =  0x010c,  // register relative address

    S_BPREL32_16t   =  0x0200,  // BP-relative
    S_LDATA32_16t   =  0x0201,  // Module-local symbol
    S_GDATA32_16t   =  0x0202,  // Global data symbol
    S_PUB32_16t     =  0x0203,  // a public symbol (CV internal reserved)
    S_LPROC32_16t   =  0x0204,  // Local procedure start
    S_GPROC32_16t   =  0x0205,  // Global procedure start
    S_THUNK32_ST    =  0x0206,  // Thunk Start
    S_BLOCK32_ST    =  0x0207,  // block start
    S_WITH32_ST     =  0x0208,  // with start
    S_LABEL32_ST    =  0x0209,  // code label
    S_CEXMODEL32    =  0x020a,  // change execution model
    S_VFTABLE32_16t =  0x020b,  // address of virtual function table
    S_REGREL32_16t  =  0x020c,  // register relative address
    S_LTHREAD32_16t =  0x020d,  // local thread storage
    S_GTHREAD32_16t =  0x020e,  // global thread storage
    S_SLINK32       =  0x020f,  // static link for MIPS EH implementation

    S_LPROCMIPS_16t =  0x0300,  // Local procedure start
    S_GPROCMIPS_16t =  0x0301,  // Global procedure start

    // if these ref symbols have names following then the names are in ST format
    S_PROCREF_ST    =  0x0400,  // Reference to a procedure
    S_DATAREF_ST    =  0x0401,  // Reference to data
    S_ALIGN         =  0x0402,  // Used for page alignment of symbols

    S_LPROCREF_ST   =  0x0403,  // Local Reference to a procedure
    S_OEM           =  0x0404,  // OEM defined symbol

    // sym records with 32-bit types embedded instead of 16-bit
    // all have 0x1000 bit set for easy identification
    // only do the 32-bit target versions since we don't really
    // care about 16-bit ones anymore.
    S_TI16_MAX          =  0x1000,

    S_REGISTER_ST   =  0x1001,  // Register variable
    S_CONSTANT_ST   =  0x1002,  // constant symbol
    S_UDT_ST        =  0x1003,  // User defined type
    S_COBOLUDT_ST   =  0x1004,  // special UDT for cobol that does not symbol pack
    S_MANYREG_ST    =  0x1005,  // multiple register variable
    S_BPREL32_ST    =  0x1006,  // BP-relative
    S_LDATA32_ST    =  0x1007,  // Module-local symbol
    S_GDATA32_ST    =  0x1008,  // Global data symbol
    S_PUB32_ST      =  0x1009,  // a public symbol (CV internal reserved)
    S_LPROC32_ST    =  0x100a,  // Local procedure start
    S_GPROC32_ST    =  0x100b,  // Global procedure start
    S_VFTABLE32     =  0x100c,  // address of virtual function table
    S_REGREL32_ST   =  0x100d,  // register relative address
    S_LTHREAD32_ST  =  0x100e,  // local thread storage
    S_GTHREAD32_ST  =  0x100f,  // global thread storage

    S_LPROCMIPS_ST  =  0x1010,  // Local procedure start
    S_GPROCMIPS_ST  =  0x1011,  // Global procedure start

    S_FRAMEPROC     =  0x1012,  // extra frame and proc information
    S_COMPILE2_ST   =  0x1013,  // extended compile flags and info

    // new symbols necessary for 16-bit enumerates of IA64 registers
    // and IA64 specific symbols

    S_MANYREG2_ST   =  0x1014,  // multiple register variable
    S_LPROCIA64_ST  =  0x1015,  // Local procedure start (IA64)
    S_GPROCIA64_ST  =  0x1016,  // Global procedure start (IA64)

    // Local symbols for IL
    S_LOCALSLOT_ST  =  0x1017,  // local IL sym with field for local slot index
    S_PARAMSLOT_ST  =  0x1018,  // local IL sym with field for parameter slot index

    S_ANNOTATION    =  0x1019,  // Annotation string literals

    // symbols to support managed code debugging
    S_GMANPROC_ST   =  0x101a,  // Global proc
    S_LMANPROC_ST   =  0x101b,  // Local proc
    S_RESERVED1     =  0x101c,  // reserved
    S_RESERVED2     =  0x101d,  // reserved
    S_RESERVED3     =  0x101e,  // reserved
    S_RESERVED4     =  0x101f,  // reserved
    S_LMANDATA_ST   =  0x1020,
    S_GMANDATA_ST   =  0x1021,
    S_MANFRAMEREL_ST=  0x1022,
    S_MANREGISTER_ST=  0x1023,
    S_MANSLOT_ST    =  0x1024,
    S_MANMANYREG_ST =  0x1025,
    S_MANREGREL_ST  =  0x1026,
    S_MANMANYREG2_ST=  0x1027,
    S_MANTYPREF     =  0x1028,  // Index for type referenced by name from metadata
    S_UNAMESPACE_ST =  0x1029,  // Using namespace

    // Symbols w/ SZ name fields. All name fields contain utf8 encoded strings.
    S_ST_MAX        =  0x1100,  // starting point for SZ name symbols

    S_OBJNAME       =  0x1101,  // path to object file name
    S_THUNK32       =  0x1102,  // Thunk Start
    S_BLOCK32       =  0x1103,  // block start
    S_WITH32        =  0x1104,  // with start
    S_LABEL32       =  0x1105,  // code label
    S_REGISTER      =  0x1106,  // Register variable
    S_CONSTANT      =  0x1107,  // constant symbol
    S_UDT           =  0x1108,  // User defined type
    S_COBOLUDT      =  0x1109,  // special UDT for cobol that does not symbol pack
    S_MANYREG       =  0x110a,  // multiple register variable
    S_BPREL32       =  0x110b,  // BP-relative
    S_LDATA32       =  0x110c,  // Module-local symbol
    S_GDATA32       =  0x110d,  // Global data symbol
    S_PUB32         =  0x110e,  // a public symbol (CV internal reserved)
    S_LPROC32       =  0x110f,  // Local procedure start
    S_GPROC32       =  0x1110,  // Global procedure start
    S_REGREL32      =  0x1111,  // register relative address
    S_LTHREAD32     =  0x1112,  // local thread storage
    S_GTHREAD32     =  0x1113,  // global thread storage

    S_LPROCMIPS     =  0x1114,  // Local procedure start
    S_GPROCMIPS     =  0x1115,  // Global procedure start
    S_COMPILE2      =  0x1116,  // extended compile flags and info
    S_MANYREG2      =  0x1117,  // multiple register variable
    S_LPROCIA64     =  0x1118,  // Local procedure start (IA64)
    S_GPROCIA64     =  0x1119,  // Global procedure start (IA64)
    S_LOCALSLOT     =  0x111a,  // local IL sym with field for local slot index
    S_SLOT          = S_LOCALSLOT,  // alias for LOCALSLOT
    S_PARAMSLOT     =  0x111b,  // local IL sym with field for parameter slot index

    // symbols to support managed code debugging
    S_LMANDATA      =  0x111c,
    S_GMANDATA      =  0x111d,
    S_MANFRAMEREL   =  0x111e,
    S_MANREGISTER   =  0x111f,
    S_MANSLOT       =  0x1120,
    S_MANMANYREG    =  0x1121,
    S_MANREGREL     =  0x1122,
    S_MANMANYREG2   =  0x1123,
    S_UNAMESPACE    =  0x1124,  // Using namespace

    // ref symbols with name fields
    S_PROCREF       =  0x1125,  // Reference to a procedure
    S_DATAREF       =  0x1126,  // Reference to data
    S_LPROCREF      =  0x1127,  // Local Reference to a procedure
    S_ANNOTATIONREF =  0x1128,  // Reference to an S_ANNOTATION symbol
    S_TOKENREF      =  0x1129,  // Reference to one of the many MANPROCSYM's

    // continuation of managed symbols
    S_GMANPROC      =  0x112a,  // Global proc
    S_LMANPROC      =  0x112b,  // Local proc

    // short, light-weight thunks
    S_TRAMPOLINE    =  0x112c,  // trampoline thunks
    S_MANCONSTANT   =  0x112d,  // constants with metadata type info

    // native attributed local/parms
    S_ATTR_FRAMEREL =  0x112e,  // relative to virtual frame ptr
    S_ATTR_REGISTER =  0x112f,  // stored in a register
    S_ATTR_REGREL   =  0x1130,  // relative to register (alternate frame ptr)
    S_ATTR_MANYREG  =  0x1131,  // stored in >1 register

    // Separated code (from the compiler) support
    S_SEPCODE       =  0x1132,

    S_LOCAL_2005    =  0x1133,  // defines a local symbol in optimized code
    S_DEFRANGE_2005 =  0x1134,  // defines a single range of addresses in which symbol can be evaluated
    S_DEFRANGE2_2005 =  0x1135,  // defines ranges of addresses in which symbol can be evaluated

    S_SECTION       =  0x1136,  // A COFF section in a PE executable
    S_COFFGROUP     =  0x1137,  // A COFF group
    S_EXPORT        =  0x1138,  // A export

    S_CALLSITEINFO  =  0x1139,  // Indirect call site information
    S_FRAMECOOKIE   =  0x113a,  // Security cookie information

    S_DISCARDED     =  0x113b,  // Discarded by LINK /OPT:REF (experimental, see richards)

    S_COMPILE3      =  0x113c,  // Replacement for S_COMPILE2
    S_ENVBLOCK      =  0x113d,  // Environment block split off from S_COMPILE2

    S_LOCAL         =  0x113e,  // defines a local symbol in optimized code
    S_DEFRANGE      =  0x113f,  // defines a single range of addresses in which symbol can be evaluated
    S_DEFRANGE_SUBFIELD =  0x1140,           // ranges for a subfield

    S_DEFRANGE_REGISTER =  0x1141,           // ranges for en-registered symbol
    S_DEFRANGE_FRAMEPOINTER_REL =  0x1142,   // range for stack symbol.
    S_DEFRANGE_SUBFIELD_REGISTER =  0x1143,  // ranges for en-registered field of symbol
    S_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE =  0x1144, // range for stack symbol span valid full scope of function body, gap might apply.
    S_DEFRANGE_REGISTER_REL =  0x1145, // range for symbol address as register + offset.

    // S_PROC symbols that reference ID instead of type
    S_LPROC32_ID     =  0x1146,
    S_GPROC32_ID     =  0x1147,
    S_LPROCMIPS_ID   =  0x1148,
    S_GPROCMIPS_ID   =  0x1149,
    S_LPROCIA64_ID   =  0x114a,
    S_GPROCIA64_ID   =  0x114b,

    S_BUILDINFO      = 0x114c, // build information.
    S_INLINESITE     = 0x114d, // inlined function callsite.
    S_INLINESITE_END = 0x114e,
    S_PROC_ID_END    = 0x114f,

    S_DEFRANGE_HLSL  = 0x1150,
    S_GDATA_HLSL     = 0x1151,
    S_LDATA_HLSL     = 0x1152,

    S_FILESTATIC     = 0x1153,

#if defined(CC_DP_CXX) && CC_DP_CXX

    S_LOCAL_DPC_GROUPSHARED = 0x1154, // DPC groupshared variable
    S_LPROC32_DPC = 0x1155, // DPC local procedure start
    S_LPROC32_DPC_ID =  0x1156,
    S_DEFRANGE_DPC_PTR_TAG =  0x1157, // DPC pointer tag definition range
    S_DPC_SYM_TAG_MAP = 0x1158, // DPC pointer tag value to symbol record map

#endif // CC_DP_CXX

    S_ARMSWITCHTABLE  = 0x1159,
    S_CALLEES = 0x115a,
    S_CALLERS = 0x115b,
    S_POGODATA = 0x115c,
    S_INLINESITE2 = 0x115d,      // extended inline site information

    S_HEAPALLOCSITE = 0x115e,    // heap allocation site

    S_MOD_TYPEREF = 0x115f,      // only generated at link time

    S_REF_MINIPDB = 0x1160,      // only generated at link time for mini PDB
    S_PDBMAP      = 0x1161,      // only generated at link time for mini PDB

    S_GDATA_HLSL32 = 0x1162,
    S_LDATA_HLSL32 = 0x1163,

    S_GDATA_HLSL32_EX = 0x1164,
    S_LDATA_HLSL32_EX = 0x1165,

    S_RECTYPE_MAX,               // one greater than last
    S_RECTYPE_LAST  = S_RECTYPE_MAX - 1,
    S_RECTYPE_PAD   = S_RECTYPE_MAX + 0x100 // Used *only* to verify symbol record types so that current PDB code can potentially read
                                // future PDBs (assuming no format change, etc).

} SYM_ENUM_e;

typedef uint32_t CV_uoff32_t;
typedef int32_t  CV_off32_t;
typedef uint16_t CV_uoff16_t;
typedef int16_t  CV_off16_t;
typedef uint16_t CV_typ16_t;
typedef uint32_t CV_typ_t;
typedef uint32_t CV_pubsymflag_t;    // must be same as CV_typ_t.
typedef uint32_t CV_tkn_t;

/**
 * Symbol record types.
 */
#pragma pack(push, 1)

typedef struct REGSYM_16t {
    uint16_t   reclen;    // Record length
    uint16_t   rectyp;    // S_REGISTER_16t
    CV_typ16_t typind;    // Type index
    uint16_t   reg;       // register enumerate
    uint8_t    name[1];   // Length-prefixed name
} REGSYM_16t;

typedef struct REGSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_REGISTER
    CV_typ_t  typind;     // Type index or Metadata token
    uint16_t  reg;        // register enumerate
    uint8_t   name[1];    // Length-prefixed name
} REGSYM;

typedef struct ATTRREGSYM {
    uint16_t     reclen;  // Record length
    uint16_t     rectyp;  // S_MANREGISTER | S_ATTR_REGISTER
    CV_typ_t     typind;  // Type index or Metadata token
    CV_lvar_attr attr;    // local var attributes
    uint16_t     reg;     // register enumerate
    uint8_t      name[1]; // Length-prefixed name
} ATTRREGSYM;

typedef struct MANYREGSYM_16t {
    uint16_t   reclen;    // Record length
    uint16_t   rectyp;    // S_MANYREG_16t
    CV_typ16_t typind;    // Type index
    uint8_t    count;     // count of number of registers
    uint8_t    reg[1];    // count register enumerates followed by
                          // length-prefixed name.  Registers are
                          // most significant first.
} MANYREGSYM_16t;

typedef struct MANYREGSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_MANYREG
    CV_typ_t  typind;     // Type index or metadata token
    uint8_t   count;      // count of number of registers
    uint8_t   reg[1];     // count register enumerates followed by
                          // length-prefixed name.  Registers are
                          // most significant first.
} MANYREGSYM;

typedef struct MANYREGSYM2 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_MANYREG2
    CV_typ_t  typind;     // Type index or metadata token
    uint16_t  count;      // count of number of registers
    uint16_t  reg[1];     // count register enumerates followed by
                          // length-prefixed name.  Registers are
                          // most significant first.
} MANYREGSYM2;

typedef struct ATTRMANYREGSYM {
    uint16_t     reclen;  // Record length
    uint16_t     rectyp;  // S_MANMANYREG
    CV_typ_t     typind;  // Type index or metadata token
    CV_lvar_attr attr;    // local var attributes
    uint8_t      count;   // count of number of registers
    uint8_t      reg[1];  // count register enumerates followed by
                          // length-prefixed name.  Registers are
                          // most significant first.
    uint8_t      name[];  // utf-8 encoded zero terminate name
} ATTRMANYREGSYM;

typedef struct ATTRMANYREGSYM2 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_MANMANYREG2 | S_ATTR_MANYREG
    CV_typ_t     typind;  // Type index or metadata token
    CV_lvar_attr attr;    // local var attributes
    uint16_t  count;      // count of number of registers
    uint16_t  reg[1];     // count register enumerates followed by
                          // length-prefixed name.  Registers are
                          // most significant first.
    uint8_t   name[];     // utf-8 encoded zero terminate name
} ATTRMANYREGSYM2;

typedef struct CONSTSYM_16t {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_CONSTANT_16t
    CV_typ16_t typind;    // Type index (containing enum if enumerate)
    uint16_t  value;      // numeric leaf containing value
    uint8_t   name[];     // Length-prefixed name
} CONSTSYM_16t;

typedef struct CONSTSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_CONSTANT or S_MANCONSTANT
    CV_typ_t  typind;     // Type index (containing enum if enumerate) or metadata token
    uint16_t  value;      // numeric leaf containing value
    uint8_t   name[];     // Length-prefixed name
} CONSTSYM;


typedef struct UDTSYM_16t {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_UDT_16t | S_COBOLUDT_16t
    CV_typ16_t typind;    // Type index
    uint8_t   name[1];    // Length-prefixed name
} UDTSYM_16t;


typedef struct UDTSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_UDT | S_COBOLUDT
    CV_typ_t  typind;     // Type index
    uint8_t   name[1];    // Length-prefixed name
} UDTSYM;

typedef struct MANTYPREF {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_MANTYPREF
    CV_typ_t  typind;     // Type index
} MANTYPREF;

typedef struct SEARCHSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_SSEARCH
    uint32_t  startsym;   // offset of the procedure
    uint16_t  seg;        // segment of symbol
} SEARCHSYM;


typedef struct CFLAGSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_COMPILE
    uint8_t   machine;    // target processor
    struct  {
        uint8_t   language    :8; // language index
        uint8_t   pcode       :1; // true if pcode present
        uint8_t   floatprec   :2; // floating precision
        uint8_t   floatpkg    :2; // float package
        uint8_t   ambdata     :3; // ambient data model
        uint8_t   ambcode     :3; // ambient code model
        uint8_t   mode32      :1; // true if compiled 32 bit mode
        uint8_t   pad         :4; // reserved
    } flags;
    uint8_t       ver[1];     // Length-prefixed compiler version string
} CFLAGSYM;


typedef struct COMPILESYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_COMPILE2
    struct {
        uint32_t   iLanguage       :  8;   // language index
        uint32_t   fEC             :  1;   // compiled for E/C
        uint32_t   fNoDbgInfo      :  1;   // not compiled with debug info
        uint32_t   fLTCG           :  1;   // compiled with LTCG
        uint32_t   fNoDataAlign    :  1;   // compiled with -Bzalign
        uint32_t   fManagedPresent :  1;   // managed code/data present
        uint32_t   fSecurityChecks :  1;   // compiled with /GS
        uint32_t   fHotPatch       :  1;   // compiled with /hotpatch
        uint32_t   fCVTCIL         :  1;   // converted with CVTCIL
        uint32_t   fMSILModule     :  1;   // MSIL netmodule
        uint32_t   pad             : 15;   // reserved, must be 0
    } flags;
    uint16_t  machine;    // target processor
    uint16_t  verFEMajor; // front end major version #
    uint16_t  verFEMinor; // front end minor version #
    uint16_t  verFEBuild; // front end build version #
    uint16_t  verMajor;   // back end major version #
    uint16_t  verMinor;   // back end minor version #
    uint16_t  verBuild;   // back end build version #
    uint8_t   verSt[1];   // Length-prefixed compiler version string, followed
                                //  by an optional block of zero terminated strings
                                //  terminated with a double zero.
} COMPILESYM;

typedef struct COMPILESYM3 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_COMPILE3
    struct {
        uint32_t   iLanguage       :  8;   // language index
        uint32_t   fEC             :  1;   // compiled for E/C
        uint32_t   fNoDbgInfo      :  1;   // not compiled with debug info
        uint32_t   fLTCG           :  1;   // compiled with LTCG
        uint32_t   fNoDataAlign    :  1;   // compiled with -Bzalign
        uint32_t   fManagedPresent :  1;   // managed code/data present
        uint32_t   fSecurityChecks :  1;   // compiled with /GS
        uint32_t   fHotPatch       :  1;   // compiled with /hotpatch
        uint32_t   fCVTCIL         :  1;   // converted with CVTCIL
        uint32_t   fMSILModule     :  1;   // MSIL netmodule
        uint32_t   fSdl            :  1;   // compiled with /sdl
        uint32_t   fPGO            :  1;   // compiled with /ltcg:pgo or pgu
        uint32_t   fExp            :  1;   // .exp module
        uint32_t   pad             : 12;   // reserved, must be 0
    } flags;
    uint16_t  machine;    // target processor
    uint16_t  verFEMajor; // front end major version #
    uint16_t  verFEMinor; // front end minor version #
    uint16_t  verFEBuild; // front end build version #
    uint16_t  verFEQFE;   // front end QFE version #
    uint16_t  verMajor;   // back end major version #
    uint16_t  verMinor;   // back end minor version #
    uint16_t  verBuild;   // back end build version #
    uint16_t  verQFE;     // back end QFE version #
    char            verSz[1];   // Zero terminated compiler version string
} COMPILESYM3;

typedef struct ENVBLOCKSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_ENVBLOCK
    struct {
        uint8_t  rev              : 1;    // reserved
        uint8_t  pad              : 7;    // reserved, must be 0
    } flags;
    uint8_t   rgsz[1];    // Sequence of zero-terminated strings
} ENVBLOCKSYM;

typedef struct OBJNAMESYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_OBJNAME
    uint32_t   signature;  // signature
    uint8_t   name[1];    // Length-prefixed name
} OBJNAMESYM;


typedef struct ENDARGSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_ENDARG
} ENDARGSYM;


typedef struct RETURNSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_RETURN
    CV_GENERIC_FLAG flags;      // flags
    uint8_t   style;      // CV_GENERIC_STYLE_e return style
                                // followed by return method data
} RETURNSYM;


typedef struct ENTRYTHISSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_ENTRYTHIS
    uint8_t   thissym;    // symbol describing this pointer on entry
} ENTRYTHISSYM;


//      symbol types for 16:16 memory model


typedef struct BPRELSYM16 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_BPREL16
    CV_off16_t      off;        // BP-relative offset
    CV_typ16_t      typind;     // Type index
    uint8_t   name[1];    // Length-prefixed name
} BPRELSYM16;


typedef struct DATASYM16 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_LDATA or S_GDATA
    CV_uoff16_t     off;        // offset of symbol
    uint16_t  seg;        // segment of symbol
    CV_typ16_t      typind;     // Type index
    uint8_t   name[1];    // Length-prefixed name
} DATASYM16;
typedef DATASYM16 PUBSYM16;


typedef struct PROCSYM16 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_GPROC16 or S_LPROC16
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint32_t   pNext;      // pointer to next symbol
    uint16_t  len;        // Proc length
    uint16_t  DbgStart;   // Debug start offset
    uint16_t  DbgEnd;     // Debug end offset
    CV_uoff16_t     off;        // offset of symbol
    uint16_t  seg;        // segment of symbol
    CV_typ16_t      typind;     // Type index
    CV_PROCFLAGS    flags;      // Proc flags
    uint8_t   name[1];    // Length-prefixed name
} PROCSYM16;


typedef struct THUNKSYM16 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_THUNK
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint32_t   pNext;      // pointer to next symbol
    CV_uoff16_t     off;        // offset of symbol
    uint16_t  seg;        // segment of symbol
    uint16_t  len;        // length of thunk
    uint8_t   ord;        // THUNK_ORDINAL specifying type of thunk
    uint8_t   name[1];    // name of thunk
    uint8_t   variant[]; // variant portion of thunk
} THUNKSYM16;

typedef struct LABELSYM16 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_LABEL16
    CV_uoff16_t     off;        // offset of symbol
    uint16_t  seg;        // segment of symbol
    CV_PROCFLAGS    flags;      // flags
    uint8_t   name[1];    // Length-prefixed name
} LABELSYM16;


typedef struct BLOCKSYM16 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_BLOCK16
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint16_t  len;        // Block length
    CV_uoff16_t     off;        // offset of symbol
    uint16_t  seg;        // segment of symbol
    uint8_t   name[1];    // Length-prefixed name
} BLOCKSYM16;


typedef struct WITHSYM16 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_WITH16
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint16_t  len;        // Block length
    CV_uoff16_t     off;        // offset of symbol
    uint16_t  seg;        // segment of symbol
    uint8_t   expr[1];    // Length-prefixed expression
} WITHSYM16;


typedef enum CEXM_MODEL_e {
    CEXM_MDL_table          = 0x00, // not executable
    CEXM_MDL_jumptable      = 0x01, // Compiler generated jump table
    CEXM_MDL_datapad        = 0x02, // Data padding for alignment
    CEXM_MDL_native         = 0x20, // native (actually not-pcode)
    CEXM_MDL_cobol          = 0x21, // cobol
    CEXM_MDL_codepad        = 0x22, // Code padding for alignment
    CEXM_MDL_code           = 0x23, // code
    CEXM_MDL_sql            = 0x30, // sql
    CEXM_MDL_pcode          = 0x40, // pcode
    CEXM_MDL_pcode32Mac     = 0x41, // macintosh 32 bit pcode
    CEXM_MDL_pcode32MacNep  = 0x42, // macintosh 32 bit pcode native entry point
    CEXM_MDL_javaInt        = 0x50,
    CEXM_MDL_unknown        = 0xff
} CEXM_MODEL_e;

typedef enum CV_COBOL_e {
    CV_COBOL_dontstop,
    CV_COBOL_pfm,
    CV_COBOL_false,
    CV_COBOL_extcall
} CV_COBOL_e;

typedef struct CEXMSYM16 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_CEXMODEL16
    CV_uoff16_t     off;        // offset of symbol
    uint16_t  seg;        // segment of symbol
    uint16_t  model;      // execution model
    union {
        struct  {
            CV_uoff16_t pcdtable;   // offset to pcode function table
            CV_uoff16_t pcdspi;     // offset to segment pcode information
        } pcode;
        struct {
            uint16_t  subtype;   // see CV_COBOL_e above
            uint16_t  flag;
        } cobol;
    };
} CEXMSYM16;


typedef struct VPATHSYM16 {
    uint16_t  reclen;     // record length
    uint16_t  rectyp;     // S_VFTPATH16
    CV_uoff16_t     off;        // offset of virtual function table
    uint16_t  seg;        // segment of virtual function table
    CV_typ16_t      root;       // type index of the root of path
    CV_typ16_t      path;       // type index of the path record
} VPATHSYM16;


typedef struct REGREL16 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_REGREL16
    CV_uoff16_t     off;        // offset of symbol
    uint16_t  reg;        // register index
    CV_typ16_t      typind;     // Type index
    uint8_t   name[1];    // Length-prefixed name
} REGREL16;


typedef struct BPRELSYM32_16t {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_BPREL32_16t
    CV_off32_t      off;        // BP-relative offset
    CV_typ16_t      typind;     // Type index
    uint8_t   name[1];    // Length-prefixed name
} BPRELSYM32_16t;

typedef struct BPRELSYM32 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_BPREL32
    CV_off32_t      off;        // BP-relative offset
    CV_typ_t        typind;     // Type index or Metadata token
    uint8_t   name[1];    // Length-prefixed name
} BPRELSYM32;

typedef struct FRAMERELSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_MANFRAMEREL | S_ATTR_FRAMEREL
    CV_off32_t      off;        // Frame relative offset
    CV_typ_t        typind;     // Type index or Metadata token
    CV_lvar_attr    attr;       // local var attributes
    uint8_t   name[1];    // Length-prefixed name
} FRAMERELSYM;

typedef FRAMERELSYM ATTRFRAMERELSYM;


typedef struct SLOTSYM32 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_LOCALSLOT or S_PARAMSLOT
    uint32_t   iSlot;      // slot index
    CV_typ_t        typind;     // Type index or Metadata token
    uint8_t   name[1];    // Length-prefixed name
} SLOTSYM32;

typedef struct ATTRSLOTSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_MANSLOT
    uint32_t   iSlot;      // slot index
    CV_typ_t        typind;     // Type index or Metadata token
    CV_lvar_attr    attr;       // local var attributes
    uint8_t   name[1];    // Length-prefixed name
} ATTRSLOTSYM;

typedef struct ANNOTATIONSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_ANNOTATION
    CV_uoff32_t     off;
    uint16_t  seg;
    uint16_t  csz;        // Count of zero terminated annotation strings
    uint8_t   rgsz[1];    // Sequence of zero terminated annotation strings
} ANNOTATIONSYM;

typedef struct DATASYM32_16t {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_LDATA32_16t, S_GDATA32_16t or S_PUB32_16t
    CV_uoff32_t     off;
    uint16_t  seg;
    CV_typ16_t      typind;     // Type index
    uint8_t   name[1];    // Length-prefixed name
} DATASYM32_16t;
typedef DATASYM32_16t PUBSYM32_16t;

typedef struct DATASYM32 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_LDATA32, S_GDATA32, S_LMANDATA, S_GMANDATA
    CV_typ_t        typind;     // Type index, or Metadata token if a managed symbol
    CV_uoff32_t     off;
    uint16_t  seg;
    uint8_t   name[1];    // Length-prefixed name
} DATASYM32;

typedef struct DATASYMHLSL {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_GDATA_HLSL, S_LDATA_HLSL
    CV_typ_t        typind;     // Type index
    uint16_t  regType;    // register type from CV_HLSLREG_e
    uint16_t  dataslot;   // Base data (cbuffer, groupshared, etc.) slot
    uint16_t  dataoff;    // Base data byte offset start
    uint16_t  texslot;    // Texture slot start
    uint16_t  sampslot;   // Sampler slot start
    uint16_t  uavslot;    // UAV slot start
    uint8_t   name[1];    // name
} DATASYMHLSL;

typedef struct DATASYMHLSL32 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_GDATA_HLSL32, S_LDATA_HLSL32
    CV_typ_t        typind;     // Type index
    uint32_t   dataslot;   // Base data (cbuffer, groupshared, etc.) slot
    uint32_t   dataoff;    // Base data byte offset start
    uint32_t   texslot;    // Texture slot start
    uint32_t   sampslot;   // Sampler slot start
    uint32_t   uavslot;    // UAV slot start
    uint16_t  regType;    // register type from CV_HLSLREG_e
    uint8_t   name[1];    // name
} DATASYMHLSL32;

typedef struct DATASYMHLSL32_EX {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_GDATA_HLSL32_EX, S_LDATA_HLSL32_EX
    CV_typ_t        typind;     // Type index
    uint32_t   regID;      // Register index
    uint32_t   dataoff;    // Base data byte offset start
    uint32_t   bindSpace;  // Binding space
    uint32_t   bindSlot;   // Lower bound in binding space
    uint16_t  regType;    // register type from CV_HLSLREG_e
    uint8_t   name[1];    // name
} DATASYMHLSL32_EX;

typedef enum CV_PUBSYMFLAGS_e
 {
    cvpsfNone     = 0,
    cvpsfCode     = 0x00000001,
    cvpsfFunction = 0x00000002,
    cvpsfManaged  = 0x00000004,
    cvpsfMSIL     = 0x00000008,
} CV_PUBSYMFLAGS_e;

typedef union CV_PUBSYMFLAGS {
    CV_pubsymflag_t grfFlags;
    struct {
        CV_pubsymflag_t fCode       :  1;    // set if public symbol refers to a code address
        CV_pubsymflag_t fFunction   :  1;    // set if public symbol is a function
        CV_pubsymflag_t fManaged    :  1;    // set if managed code (native or IL)
        CV_pubsymflag_t fMSIL       :  1;    // set if managed IL code
        CV_pubsymflag_t __unused    : 28;    // must be zero
    };
} CV_PUBSYMFLAGS;

typedef struct PUBSYM32 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_PUB32
    CV_PUBSYMFLAGS  pubsymflags;
    CV_uoff32_t     off;
    uint16_t  seg;
    uint8_t   name[1];    // Length-prefixed name
} PUBSYM32;


typedef struct PROCSYM32_16t {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_GPROC32_16t or S_LPROC32_16t
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint32_t   pNext;      // pointer to next symbol
    uint32_t   len;        // Proc length
    uint32_t   DbgStart;   // Debug start offset
    uint32_t   DbgEnd;     // Debug end offset
    CV_uoff32_t     off;
    uint16_t  seg;
    CV_typ16_t      typind;     // Type index
    CV_PROCFLAGS    flags;      // Proc flags
    uint8_t   name[1];    // Length-prefixed name
} PROCSYM32_16t;

typedef struct PROCSYM32 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_GPROC32, S_LPROC32, S_GPROC32_ID, S_LPROC32_ID, S_LPROC32_DPC or S_LPROC32_DPC_ID
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint32_t   pNext;      // pointer to next symbol
    uint32_t   len;        // Proc length
    uint32_t   DbgStart;   // Debug start offset
    uint32_t   DbgEnd;     // Debug end offset
    CV_typ_t        typind;     // Type index or ID
    CV_uoff32_t     off;
    uint16_t  seg;
    CV_PROCFLAGS    flags;      // Proc flags
    uint8_t   name[1];    // Length-prefixed name
} PROCSYM32;

typedef struct MANPROCSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_GMANPROC, S_LMANPROC, S_GMANPROCIA64 or S_LMANPROCIA64
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint32_t   pNext;      // pointer to next symbol
    uint32_t   len;        // Proc length
    uint32_t   DbgStart;   // Debug start offset
    uint32_t   DbgEnd;     // Debug end offset
    CV_tkn_t        token;      // COM+ metadata token for method
    CV_uoff32_t     off;
    uint16_t  seg;
    CV_PROCFLAGS    flags;      // Proc flags
    uint16_t  retReg;     // Register return value is in (may not be used for all archs)
    uint8_t   name[1];    // optional name field
} MANPROCSYM;

typedef struct MANPROCSYMMIPS {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_GMANPROCMIPS or S_LMANPROCMIPS
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint32_t   pNext;      // pointer to next symbol
    uint32_t   len;        // Proc length
    uint32_t   DbgStart;   // Debug start offset
    uint32_t   DbgEnd;     // Debug end offset
    uint32_t   regSave;    // int register save mask
    uint32_t   fpSave;     // fp register save mask
    CV_uoff32_t     intOff;     // int register save offset
    CV_uoff32_t     fpOff;      // fp register save offset
    CV_tkn_t        token;      // COM+ token type
    CV_uoff32_t     off;
    uint16_t  seg;
    uint8_t   retReg;     // Register return value is in
    uint8_t   frameReg;   // Frame pointer register
    uint8_t   name[1];    // optional name field
} MANPROCSYMMIPS;

typedef struct THUNKSYM32 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_THUNK32
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint32_t   pNext;      // pointer to next symbol
    CV_uoff32_t     off;
    uint16_t  seg;
    uint16_t  len;        // length of thunk
    uint8_t   ord;        // THUNK_ORDINAL specifying type of thunk
    uint8_t   name[1];    // Length-prefixed name
    uint8_t   variant[]; // variant portion of thunk
} THUNKSYM32;

typedef enum TRAMP_e {      // Trampoline subtype
    trampIncremental,           // incremental thunks
    trampBranchIsland,          // Branch island thunks
} TRAMP_e;

typedef struct TRAMPOLINESYM {  // Trampoline thunk symbol
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_TRAMPOLINE
    uint16_t  trampType;  // trampoline sym subtype
    uint16_t  cbThunk;    // size of the thunk
    CV_uoff32_t     offThunk;   // offset of the thunk
    CV_uoff32_t     offTarget;  // offset of the target of the thunk
    uint16_t  sectThunk;  // section index of the thunk
    uint16_t  sectTarget; // section index of the target of the thunk
} TRAMPOLINE;

typedef struct LABELSYM32 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_LABEL32
    CV_uoff32_t     off;
    uint16_t  seg;
    CV_PROCFLAGS    flags;      // flags
    uint8_t   name[1];    // Length-prefixed name
} LABELSYM32;


typedef struct BLOCKSYM32 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_BLOCK32
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint32_t   len;        // Block length
    CV_uoff32_t     off;        // Offset in code segment
    uint16_t  seg;        // segment of label
    uint8_t   name[1];    // Length-prefixed name
} BLOCKSYM32;


typedef struct WITHSYM32 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_WITH32
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint32_t   len;        // Block length
    CV_uoff32_t     off;        // Offset in code segment
    uint16_t  seg;        // segment of label
    uint8_t   expr[1];    // Length-prefixed expression string
} WITHSYM32;



typedef struct CEXMSYM32 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_CEXMODEL32
    CV_uoff32_t     off;        // offset of symbol
    uint16_t  seg;        // segment of symbol
    uint16_t  model;      // execution model
    union {
        struct  {
            CV_uoff32_t pcdtable;   // offset to pcode function table
            CV_uoff32_t pcdspi;     // offset to segment pcode information
        } pcode;
        struct {
            uint16_t  subtype;   // see CV_COBOL_e above
            uint16_t  flag;
        } cobol;
        struct {
            CV_uoff32_t calltableOff; // offset to function table
            uint16_t calltableSeg; // segment of function table
        } pcode32Mac;
    };
} CEXMSYM32;



typedef struct VPATHSYM32_16t {
    uint16_t  reclen;     // record length
    uint16_t  rectyp;     // S_VFTABLE32_16t
    CV_uoff32_t     off;        // offset of virtual function table
    uint16_t  seg;        // segment of virtual function table
    CV_typ16_t      root;       // type index of the root of path
    CV_typ16_t      path;       // type index of the path record
} VPATHSYM32_16t;

typedef struct VPATHSYM32 {
    uint16_t  reclen;     // record length
    uint16_t  rectyp;     // S_VFTABLE32
    CV_typ_t        root;       // type index of the root of path
    CV_typ_t        path;       // type index of the path record
    CV_uoff32_t     off;        // offset of virtual function table
    uint16_t  seg;        // segment of virtual function table
} VPATHSYM32;





typedef struct REGREL32_16t {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_REGREL32_16t
    CV_uoff32_t     off;        // offset of symbol
    uint16_t  reg;        // register index for symbol
    CV_typ16_t      typind;     // Type index
    uint8_t   name[1];    // Length-prefixed name
} REGREL32_16t;

typedef struct REGREL32 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_REGREL32
    CV_uoff32_t     off;        // offset of symbol
    CV_typ_t        typind;     // Type index or metadata token
    uint16_t  reg;        // register index for symbol
    uint8_t   name[1];    // Length-prefixed name
} REGREL32;

typedef struct ATTRREGREL {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_MANREGREL | S_ATTR_REGREL
    CV_uoff32_t     off;        // offset of symbol
    CV_typ_t        typind;     // Type index or metadata token
    uint16_t  reg;        // register index for symbol
    CV_lvar_attr    attr;       // local var attributes
    uint8_t   name[1];    // Length-prefixed name
} ATTRREGREL;

typedef ATTRREGREL  ATTRREGRELSYM;

typedef struct THREADSYM32_16t {
    uint16_t  reclen;     // record length
    uint16_t  rectyp;     // S_LTHREAD32_16t | S_GTHREAD32_16t
    CV_uoff32_t     off;        // offset into thread storage
    uint16_t  seg;        // segment of thread storage
    CV_typ16_t      typind;     // type index
    uint8_t   name[1];    // length prefixed name
} THREADSYM32_16t;

typedef struct THREADSYM32 {
    uint16_t  reclen;     // record length
    uint16_t  rectyp;     // S_LTHREAD32 | S_GTHREAD32
    CV_typ_t        typind;     // type index
    CV_uoff32_t     off;        // offset into thread storage
    uint16_t  seg;        // segment of thread storage
    uint8_t   name[1];    // length prefixed name
} THREADSYM32;

typedef struct SLINK32 {
    uint16_t  reclen;     // record length
    uint16_t  rectyp;     // S_SLINK32
    uint32_t   framesize;  // frame size of parent procedure
    CV_off32_t      off;        // signed offset where the static link was saved relative to the value of reg
    uint16_t  reg;
} SLINK32;

typedef struct PROCSYMMIPS_16t {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_GPROCMIPS_16t or S_LPROCMIPS_16t
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint32_t   pNext;      // pointer to next symbol
    uint32_t   len;        // Proc length
    uint32_t   DbgStart;   // Debug start offset
    uint32_t   DbgEnd;     // Debug end offset
    uint32_t   regSave;    // int register save mask
    uint32_t   fpSave;     // fp register save mask
    CV_uoff32_t     intOff;     // int register save offset
    CV_uoff32_t     fpOff;      // fp register save offset
    CV_uoff32_t     off;        // Symbol offset
    uint16_t  seg;        // Symbol segment
    CV_typ16_t      typind;     // Type index
    uint8_t   retReg;     // Register return value is in
    uint8_t   frameReg;   // Frame pointer register
    uint8_t   name[1];    // Length-prefixed name
} PROCSYMMIPS_16t;

typedef struct PROCSYMMIPS {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_GPROCMIPS or S_LPROCMIPS
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint32_t   pNext;      // pointer to next symbol
    uint32_t   len;        // Proc length
    uint32_t   DbgStart;   // Debug start offset
    uint32_t   DbgEnd;     // Debug end offset
    uint32_t   regSave;    // int register save mask
    uint32_t   fpSave;     // fp register save mask
    CV_uoff32_t     intOff;     // int register save offset
    CV_uoff32_t     fpOff;      // fp register save offset
    CV_typ_t        typind;     // Type index
    CV_uoff32_t     off;        // Symbol offset
    uint16_t  seg;        // Symbol segment
    uint8_t   retReg;     // Register return value is in
    uint8_t   frameReg;   // Frame pointer register
    uint8_t   name[1];    // Length-prefixed name
} PROCSYMMIPS;

typedef struct PROCSYMIA64 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_GPROCIA64 or S_LPROCIA64
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint32_t   pNext;      // pointer to next symbol
    uint32_t   len;        // Proc length
    uint32_t   DbgStart;   // Debug start offset
    uint32_t   DbgEnd;     // Debug end offset
    CV_typ_t        typind;     // Type index
    CV_uoff32_t     off;        // Symbol offset
    uint16_t  seg;        // Symbol segment
    uint16_t  retReg;     // Register return value is in
    CV_PROCFLAGS    flags;      // Proc flags
    uint8_t   name[1];    // Length-prefixed name
} PROCSYMIA64;

typedef struct REFSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_PROCREF_ST, S_DATAREF_ST, or S_LPROCREF_ST
    uint32_t   sumName;    // SUC of the name
    uint32_t   ibSym;      // Offset of actual symbol in $$Symbols
    uint16_t  imod;       // Module containing the actual symbol
    uint16_t  usFill;     // align this record
} REFSYM;

typedef struct REFSYM2 {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_PROCREF, S_DATAREF, or S_LPROCREF
    uint32_t   sumName;    // SUC of the name
    uint32_t   ibSym;      // Offset of actual symbol in $$Symbols
    uint16_t  imod;       // Module containing the actual symbol
    uint8_t   name[1];    // hidden name made a first class member
} REFSYM2;

typedef struct ALIGNSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_ALIGN
} ALIGNSYM;

typedef struct OEMSYMBOL {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_OEM
    uint8_t   idOem[16];  // an oem ID (GUID)
    CV_typ_t        typind;     // Type index
    uint32_t   rgl[];      // user data, force 4-byte alignment
} OEMSYMBOL;

//  generic block definition symbols
//  these are similar to the equivalent 16:16 or 16:32 symbols but
//  only define the length, type and linkage fields

typedef struct PROCSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_GPROC16 or S_LPROC16
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint32_t   pNext;      // pointer to next symbol
} PROCSYM;


typedef struct THUNKSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_THUNK
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
    uint32_t   pNext;      // pointer to next symbol
} THUNKSYM;

typedef struct BLOCKSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_BLOCK16
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
} BLOCKSYM;


typedef struct WITHSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_WITH16
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this blocks end
} WITHSYM;

typedef struct FRAMEPROCSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_FRAMEPROC
    uint32_t   cbFrame;    // count of bytes of total frame of procedure
    uint32_t   cbPad;      // count of bytes of padding in the frame
    CV_uoff32_t     offPad;     // offset (relative to frame poniter) to where
                                //  padding starts
    uint32_t   cbSaveRegs; // count of bytes of callee save registers
    CV_uoff32_t     offExHdlr;  // offset of exception handler
    uint16_t  sectExHdlr; // section id of exception handler

    struct {
        uint32_t   fHasAlloca  :  1;   // function uses _alloca()
        uint32_t   fHasSetJmp  :  1;   // function uses setjmp()
        uint32_t   fHasLongJmp :  1;   // function uses longjmp()
        uint32_t   fHasInlAsm  :  1;   // function uses inline asm
        uint32_t   fHasEH      :  1;   // function has EH states
        uint32_t   fInlSpec    :  1;   // function was speced as inline
        uint32_t   fHasSEH     :  1;   // function has SEH
        uint32_t   fNaked      :  1;   // function is __declspec(naked)
        uint32_t   fSecurityChecks :  1;   // function has buffer security check introduced by /GS.
        uint32_t   fAsyncEH    :  1;   // function compiled with /EHa
        uint32_t   fGSNoStackOrdering :  1;   // function has /GS buffer checks, but stack ordering couldn't be done
        uint32_t   fWasInlined :  1;   // function was inlined within another function
        uint32_t   fGSCheck    :  1;   // function is __declspec(strict_gs_check)
        uint32_t   fSafeBuffers : 1;   // function is __declspec(safebuffers)
        uint32_t   encodedLocalBasePointer : 2;  // record function's local pointer explicitly.
        uint32_t   encodedParamBasePointer : 2;  // record function's parameter pointer explicitly.
        uint32_t   fPogoOn      : 1;   // function was compiled with PGO/PGU
        uint32_t   fValidCounts : 1;   // Do we have valid Pogo counts?
        uint32_t   fOptSpeed    : 1;  // Did we optimize for speed?
        uint32_t   fGuardCF    :  1;   // function contains CFG checks (and no write checks)
        uint32_t   fGuardCFW   :  1;   // function contains CFW checks and/or instrumentation
        uint32_t   pad          : 9;   // must be zero
    } flags;
} FRAMEPROCSYM;

typedef struct UNAMESPACE {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_UNAMESPACE
    uint8_t   name[1];    // name
} UNAMESPACE;

typedef struct SEPCODESYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_SEPCODE
    uint32_t   pParent;    // pointer to the parent
    uint32_t   pEnd;       // pointer to this block's end
    uint32_t   length;     // count of bytes of this block
    CV_SEPCODEFLAGS scf;        // flags
    CV_uoff32_t     off;        // sect:off of the separated code
    CV_uoff32_t     offParent;  // sectParent:offParent of the enclosing scope
    uint16_t  sect;       //  (proc, block, or sepcode)
    uint16_t  sectParent;
} SEPCODESYM;

typedef struct BUILDINFOSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_BUILDINFO
    CV_ItemId       id;         // CV_ItemId of Build Info.
} BUILDINFOSYM;

typedef struct INLINESITESYM {
    uint16_t  reclen;    // Record length
    uint16_t  rectyp;    // S_INLINESITE
    uint32_t   pParent;   // pointer to the inliner
    uint32_t   pEnd;      // pointer to this block's end
    CV_ItemId       inlinee;   // CV_ItemId of inlinee
    uint8_t   binaryAnnotations[];   // an array of compressed binary annotations.
} INLINESITESYM;

typedef struct INLINESITESYM2 {
    uint16_t  reclen;         // Record length
    uint16_t  rectyp;         // S_INLINESITE2
    uint32_t   pParent;        // pointer to the inliner
    uint32_t   pEnd;           // pointer to this block's end
    CV_ItemId       inlinee;        // CV_ItemId of inlinee
    uint32_t   invocations;    // entry count
    uint8_t   binaryAnnotations[];   // an array of compressed binary annotations.
} INLINESITESYM2;


// Defines a locals and it is live range, how to evaluate.
// S_DEFRANGE modifies previous local S_LOCAL, it has to consecutive.

typedef struct LOCALSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_LOCAL
    CV_typ_t        typind;     // type index
    CV_LVARFLAGS    flags;      // local var flags

    uint8_t   name[];   // Name of this symbol, a null terminated array of UTF8 characters.
} LOCALSYM;

typedef struct FILESTATICSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_FILESTATIC
    CV_typ_t        typind;     // type index
    CV_uoff32_t     modOffset;  // index of mod filename in stringtable
    CV_LVARFLAGS    flags;      // local var flags

    uint8_t   name[];   // Name of this symbol, a null terminated array of UTF8 characters
} FILESTATICSYM;

typedef struct DEFRANGESYM {    // A live range of sub field of variable
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_DEFRANGE

    CV_uoff32_t     program;    // DIA program to evaluate the value of the symbol

    CV_LVAR_ADDR_RANGE range;   // Range of addresses where this program is valid
    CV_LVAR_ADDR_GAP   gaps[];  // The value is not available in following gaps.
} DEFRANGESYM;

typedef struct DEFRANGESYMSUBFIELD { // A live range of sub field of variable. like locala.i
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_DEFRANGE_SUBFIELD

    CV_uoff32_t     program;    // DIA program to evaluate the value of the symbol

    CV_uoff32_t     offParent;  // Offset in parent variable.

    CV_LVAR_ADDR_RANGE range;   // Range of addresses where this program is valid
    CV_LVAR_ADDR_GAP   gaps[];  // The value is not available in following gaps.
} DEFRANGESYMSUBFIELD;

typedef struct CV_RANGEATTR {
    uint16_t  maybe : 1;    // May have no user name on one of control flow path.
    uint16_t  padding : 15; // Padding for future use.
} CV_RANGEATTR;

typedef struct DEFRANGESYMREGISTER {    // A live range of en-registed variable
    uint16_t     reclen;     // Record length
    uint16_t     rectyp;     // S_DEFRANGE_REGISTER
    uint16_t     reg;        // Register to hold the value of the symbol
    CV_RANGEATTR       attr;       // Attribute of the register range.
    CV_LVAR_ADDR_RANGE range;      // Range of addresses where this program is valid
    CV_LVAR_ADDR_GAP   gaps[];  // The value is not available in following gaps.
} DEFRANGESYMREGISTER;

typedef struct DEFRANGESYMFRAMEPOINTERREL {    // A live range of frame variable
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_DEFRANGE_FRAMEPOINTER_REL

    CV_off32_t      offFramePointer;  // offset to frame pointer

    CV_LVAR_ADDR_RANGE range;   // Range of addresses where this program is valid
    CV_LVAR_ADDR_GAP   gaps[];  // The value is not available in following gaps.
} DEFRANGESYMFRAMEPOINTERREL;

typedef struct DEFRANGESYMFRAMEPOINTERREL_FULL_SCOPE { // A frame variable valid in all function scope
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_DEFRANGE_FRAMEPOINTER_REL

    CV_off32_t      offFramePointer;  // offset to frame pointer
} DEFRANGESYMFRAMEPOINTERREL_FULL_SCOPE;

#define CV_OFFSET_PARENT_LENGTH_LIMIT 12

// Note DEFRANGESYMREGISTERREL and DEFRANGESYMSUBFIELDREGISTER had same layout.
typedef struct DEFRANGESYMSUBFIELDREGISTER { // A live range of sub field of variable. like locala.i
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_DEFRANGE_SUBFIELD_REGISTER

    uint16_t     reg;        // Register to hold the value of the symbol
    CV_RANGEATTR       attr;       // Attribute of the register range.
    CV_uoff32_t        offParent : CV_OFFSET_PARENT_LENGTH_LIMIT;  // Offset in parent variable.
    CV_uoff32_t        padding   : 20;  // Padding for future use.
    CV_LVAR_ADDR_RANGE range;   // Range of addresses where this program is valid
    CV_LVAR_ADDR_GAP   gaps[];  // The value is not available in following gaps.
} DEFRANGESYMSUBFIELDREGISTER;

// Note DEFRANGESYMREGISTERREL and DEFRANGESYMSUBFIELDREGISTER had same layout.
// Used when /GS Copy parameter as local variable or other variable don't cover by FRAMERELATIVE.
typedef struct DEFRANGESYMREGISTERREL {    // A live range of variable related to a register.
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_DEFRANGE_REGISTER_REL

    uint16_t  baseReg;         // Register to hold the base pointer of the symbol
    uint16_t  spilledUdtMember : 1;   // Spilled member for s.i.
    uint16_t  padding          : 3;   // Padding for future use.
    uint16_t  offsetParent     : CV_OFFSET_PARENT_LENGTH_LIMIT;  // Offset in parent variable.
    CV_off32_t      offBasePointer;  // offset to register

    CV_LVAR_ADDR_RANGE range;   // Range of addresses where this program is valid
    CV_LVAR_ADDR_GAP   gaps[];  // The value is not available in following gaps.
} DEFRANGESYMREGISTERREL;

typedef struct DEFRANGESYMHLSL {    // A live range of variable related to a symbol in HLSL code.
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_DEFRANGE_HLSL or S_DEFRANGE_DPC_PTR_TAG

    uint16_t  regType;    // register type from CV_HLSLREG_e

    uint16_t  regIndices       : 2;   // 0, 1 or 2, dimensionality of register space
    uint16_t  spilledUdtMember : 1;   // this is a spilled member
    uint16_t  memorySpace      : 4;   // memory space
    uint16_t  padding          : 9;   // for future use

    uint16_t  offsetParent;           // Offset in parent variable.
    uint16_t  sizeInParent;           // Size of enregistered portion

    CV_LVAR_ADDR_RANGE range;               // Range of addresses where this program is valid
    uint8_t   data[];       // variable length data specifying gaps where the value is not available
                                            // followed by multi-dimensional offset of variable location in register
                                            // space (see CV_DEFRANGESYMHLSL_* macros below)
} DEFRANGESYMHLSL;

#if defined(CC_DP_CXX) && CC_DP_CXX

// Defines a local DPC group shared variable and its location.
typedef struct LOCALDPCGROUPSHAREDSYM {
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_LOCAL_DPC_GROUPSHARED
    CV_typ_t        typind;     // type index
    CV_LVARFLAGS    flags;      // local var flags

    uint16_t  dataslot;   // Base data (cbuffer, groupshared, etc.) slot
    uint16_t  dataoff;    // Base data byte offset start

    uint8_t   name[];   // Name of this symbol, a null terminated array of UTF8 characters.
} LOCALDPCGROUPSHAREDSYM;

typedef struct DPCSYMTAGMAP {   // A map for DPC pointer tag values to symbol records.
    uint16_t  reclen;     // Record length
    uint16_t  rectyp;     // S_DPC_SYM_TAG_MAP

    CV_DPC_SYM_TAG_MAP_ENTRY mapEntries[];  // Array of mappings from DPC pointer tag values to symbol record offsets
} DPCSYMTAGMAP;

#define CV_DPCSYMTAGMAP_COUNT(x) \
    (((x)->reclen + sizeof((x)->reclen) - sizeof(DPCSYMTAGMAP)) / sizeof(CV_DPC_SYM_TAG_MAP_ENTRY))

#endif // CC_DP_CXX

typedef enum CV_armswitchtype {
    CV_SWT_INT1         = 0,
    CV_SWT_UINT1        = 1,
    CV_SWT_INT2         = 2,
    CV_SWT_UINT2        = 3,
    CV_SWT_INT4         = 4,
    CV_SWT_UINT4        = 5,
    CV_SWT_POINTER      = 6,
    CV_SWT_UINT1SHL1    = 7,
    CV_SWT_UINT2SHL1    = 8,
    CV_SWT_INT1SHL1     = 9,
    CV_SWT_INT2SHL1     = 10,
    CV_SWT_TBB          = CV_SWT_UINT1SHL1,
    CV_SWT_TBH          = CV_SWT_UINT2SHL1,
} CV_armswitchtype;

typedef struct FUNCTIONLIST {
    uint16_t  reclen;             // Record length
    uint16_t  rectyp;             // S_CALLERS or S_CALLEES

    uint32_t   count;              // Number of functions
    CV_typ_t        funcs[];  // List of functions, dim == count
    // uint32_t   invocations[]; Followed by a parallel array of
    // invocation counts. Counts > reclen are assumed to be zero
} FUNCTIONLIST;

typedef struct POGOINFO {
    uint16_t  reclen;             // Record length
    uint16_t  rectyp;             // S_POGODATA

    uint32_t   invocations;        // Number of times function was called
    __int64         dynCount;           // Dynamic instruction count
    uint32_t   numInstrs;          // Static instruction count
    uint32_t   staInstLive;        // Final static instruction count (post inlining)
} POGOINFO;

typedef struct ARMSWITCHTABLE {
    uint16_t  reclen;             // Record length
    uint16_t  rectyp;             // S_ARMSWITCHTABLE

    CV_uoff32_t     offsetBase;         // Section-relative offset to the base for switch offsets
    uint16_t  sectBase;           // Section index of the base for switch offsets
    uint16_t  switchType;         // type of each entry
    CV_uoff32_t     offsetBranch;       // Section-relative offset to the table branch instruction
    CV_uoff32_t     offsetTable;        // Section-relative offset to the start of the table
    uint16_t  sectBranch;         // Section index of the table branch instruction
    uint16_t  sectTable;          // Section index of the table
    uint32_t   cEntries;           // number of switch table entries
} ARMSWITCHTABLE;

typedef struct MODTYPEREF {
    uint16_t  reclen;             // Record length
    uint16_t  rectyp;             // S_MOD_TYPEREF

    uint32_t   fNone     : 1;      // module doesn't reference any type
    uint32_t   fRefTMPCT : 1;      // reference /Z7 PCH types
    uint32_t   fOwnTMPCT : 1;      // module contains /Z7 PCH types
    uint32_t   fOwnTMR   : 1;      // module contains type info (/Z7)
    uint32_t   fOwnTM    : 1;      // module contains type info (/Zi or /ZI)
    uint32_t   fRefTM    : 1;      // module references type info owned by other module
    uint32_t   reserved  : 9;

    uint16_t  word0;              // these two words contain SN or module index depending
    uint16_t  word1;              // on above flags
} MODTYPEREF;

typedef struct SECTIONSYM {
    uint16_t  reclen;             // Record length
    uint16_t  rectyp;             // S_SECTION

    uint16_t  isec;               // Section number
    uint8_t   align;              // Alignment of this section (power of 2)
    uint8_t   bReserved;          // Reserved.  Must be zero.
    uint32_t   rva;
    uint32_t   cb;
    uint32_t   characteristics;
    uint8_t   name[1];            // name
} SECTIONSYM;

typedef struct COFFGROUPSYM {
    uint16_t  reclen;             // Record length
    uint16_t  rectyp;             // S_COFFGROUP

    uint32_t   cb;
    uint32_t   characteristics;
    CV_uoff32_t     off;                // Symbol offset
    uint16_t  seg;                // Symbol segment
    uint8_t   name[1];            // name
} COFFGROUPSYM;

typedef struct EXPORTSYM {
    uint16_t  reclen;             // Record length
    uint16_t  rectyp;             // S_EXPORT

    uint16_t  ordinal;
    uint16_t  fConstant : 1;      // CONSTANT
    uint16_t  fData : 1;          // DATA
    uint16_t  fPrivate : 1;       // PRIVATE
    uint16_t  fNoName : 1;        // NONAME
    uint16_t  fOrdinal : 1;       // Ordinal was explicitly assigned
    uint16_t  fForwarder : 1;     // This is a forwarder
    uint16_t  reserved : 10;      // Reserved. Must be zero.
    uint8_t   name[1];            // name of
} EXPORTSYM;

//
// Symbol for describing indirect calls when they are using
// a function pointer cast on some other type or temporary.
// Typical content will be an LF_POINTER to an LF_PROCEDURE
// type record that should mimic an actual variable with the
// function pointer type in question.
//
// Since the compiler can sometimes tail-merge a function call
// through a function pointer, there may be more than one
// S_CALLSITEINFO record at an address.  This is similar to what
// you could do in your own code by:
//
//  if (expr)
//      pfn = &function1;
//  else
//      pfn = &function2;
//
//  (*pfn)(arg list);
//

typedef struct CALLSITEINFO {
    uint16_t  reclen;             // Record length
    uint16_t  rectyp;             // S_CALLSITEINFO
    CV_off32_t      off;                // offset of call site
    uint16_t  sect;               // section index of call site
    uint16_t  __reserved_0;       // alignment padding field, must be zero
    CV_typ_t        typind;             // type index describing function signature
} CALLSITEINFO;

typedef struct HEAPALLOCSITE {
    uint16_t  reclen;             // Record length
    uint16_t  rectyp;             // S_HEAPALLOCSITE
    CV_off32_t      off;                // offset of call site
    uint16_t  sect;               // section index of call site
    uint16_t  cbInstr;            // length of heap allocation call instruction
    CV_typ_t        typind;             // type index describing function signature
} HEAPALLOCSITE;

// Frame cookie information

typedef enum CV_cookietype_e
{
   CV_COOKIETYPE_COPY = 0,
   CV_COOKIETYPE_XOR_SP,
   CV_COOKIETYPE_XOR_BP,
   CV_COOKIETYPE_XOR_R13,
} CV_cookietype_e;

// Symbol for describing security cookie's position and type
// (raw, xor'd with esp, xor'd with ebp).

typedef struct FRAMECOOKIE {
    uint16_t  reclen;             // Record length
    uint16_t  rectyp;             // S_FRAMECOOKIE
    CV_off32_t      off;                // Frame relative offset
    uint16_t  reg;                // Register index
    CV_cookietype_e cookietype;         // Type of the cookie
    uint8_t   flags;              // Flags describing this cookie
} FRAMECOOKIE;

typedef enum CV_DISCARDED_e
{
   CV_DISCARDED_UNKNOWN,
   CV_DISCARDED_NOT_SELECTED,
   CV_DISCARDED_NOT_REFERENCED,
} CV_DISCARDED_e;

typedef struct DISCARDEDSYM {
    uint16_t  reclen;             // Record length
    uint16_t  rectyp;             // S_DISCARDED
    uint32_t   discarded : 8;      // CV_DISCARDED_e
    uint32_t   reserved : 24;      // Unused
    uint32_t   fileid;             // First FILEID if line number info present
    uint32_t   linenum;            // First line number
    char            data[];   // Original record(s) with invalid type indices
} DISCARDEDSYM;

typedef struct REFMINIPDB {
    uint16_t  reclen;             // Record length
    uint16_t  rectyp;             // S_REF_MINIPDB
    union {
        uint32_t  isectCoff;       // coff section
        CV_typ_t       typind;          // type index
    };
    uint16_t  imod;               // mod index
    uint16_t  fLocal   :  1;      // reference to local (vs. global) func or data
    uint16_t  fData    :  1;      // reference to data (vs. func)
    uint16_t  fUDT     :  1;      // reference to UDT
    uint16_t  fLabel   :  1;      // reference to label
    uint16_t  fConst   :  1;      // reference to const
    uint16_t  reserved : 11;      // reserved, must be zero
    uint8_t   name[1];            // zero terminated name string
} REFMINIPDB;

typedef struct PDBMAP {
    uint16_t  reclen;             // Record length
    uint16_t  rectyp;             // S_PDBMAP
    uint8_t   name[];   // zero terminated source PDB filename followed by zero
                                        // terminated destination PDB filename, both in wchar_t
} PDBMAP;

#pragma pack(pop)

#if defined(_WIN32) && defined(__cplusplus)
#   pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif
