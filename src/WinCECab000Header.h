#include <stdint.h>
#include "WinCEArchitecture.h"

#define CE_CAB_000_HEADER_SIGNATURE 0x4543534D

typedef struct _CE_CAB_000_HEADER
{
    /** An ASCII signature, "MSCE". This is 0x4543534D as a little-endian integer */
    uint32_t AsciiSignature;
    /** Unknown purpose, usually 0 */
    uint32_t Unknown1;
    /** The overall length of this .000 header file, in bytes */
    uint32_t FileLength;
    /** Unknown purpose, usually 0 */
    uint32_t Unknown2;
    /** Unknown purpose, usually 1 */
    uint32_t Unknown3;
    /** Target architecture for this cabinet: see Appendix A */
    uint32_t TargetArchitecture;
    /** Minimal version of WinCE (major version number) required to install this cabinet, or 0 to indicate no restriction */
    uint32_t MinCEVersionMajor;
    /** Minimal version of WinCE (minor version number) required to install this cabinet, or 0 to indicate no restriction */
    uint32_t MinCEVersionMinor;
    /** Maximal version of WinCE (major version number) required to install this cabinet, or 0 to indicate no restriction */
    uint32_t MaxCEVersionMajor;
    /** Maximal version of WinCE (minor version number) required to install this cabinet, or 0 to indicate no restriction */
    uint32_t MaxCEVersionMinor;
    /** Minmal version of WinCE (build number) required to install this cabinet, or 0 to indicate no restriction */
    uint32_t MinCEBuildNumber;
    /** Maximal version of WinCE (build number) required to install this cabinet, or 0 to indicate no restriction */
    uint32_t MaxCEBuildNumber;
    /** The number of entries in the STRINGS section */
    uint16_t NumEntriesString;
    /** The number of entries in the DIRS section */
    uint16_t NumEntriesDirs;
    /** The number of entries in the FILES section */
    uint16_t NumEntriesFiles;
    /** The number of entries in the REGHIVES section */
    uint16_t NumEntriesRegHives;
    /** The number of entries in the REGKEYS section */
    uint16_t NumEntriesRegKeys;
    /** The number of entries in the LINKS section */
    uint16_t NumEntriesLinks;
    /** The file offset of the STRINGS section in bytes */
    uint32_t OffsetStrings;
    /** The file offset of the DIRS section in bytes */
    uint32_t OffsetDirs;
    /** The file offset of the FILES section in bytes */
    uint32_t OffsetFiles;
    /** The file offset of the REGHIVES section in bytes */
    uint32_t OffsetRegHives;
    /** The file offset of the REGKEYS section in bytes */
    uint32_t OffsetRegKeys;
    /** The file offset of the LINKS section, in bytes */
    uint32_t OffsetLinks;
    /** The file offset of the APPNAME string in bytes */
    uint16_t OffsetAppname;
    /** The length of the APPNAME string in bytes, including null terminating byte */
    uint16_t LengthAppname;
    /** The file offset of the PROVIDER string in bytes */
    uint16_t OffsetProvider;
    /** The length of the PROVIDER string in bytes, including null terminating byte */
    uint16_t LengthProvider;
    /** The file offset of the UNSUPPORTED multi string in bytes */
    uint16_t OffsetUnsupported;
    /** The length of the UNSUPPORTED multi string in bytes, including null terminating byte */
    uint16_t LengthUnsupported;
    /** Unknown purpose, usually 0 */
    uint16_t Unknown4;
    /** Unknown purpose, usually 0 */
    uint16_t Unknown5;
} CE_CAB_000_HEADER;

/* Target Architectures */

/** No specific architecture */
#define CE_CAB_000_ARCH_UNDEFINED 0
#define CE_CAB_000_ARCH_UNDEFINED_NAME "UNKNOWN"
/** SHx SH3 */
#define CE_CAB_000_ARCH_SH3 103
#define CE_CAB_000_ARCH_SH3_NAME CE_ARCH_SH3
/** SHx SH4 */
#define CE_CAB_000_ARCH_SH4 104
#define CE_CAB_000_ARCH_SH4_NAME CE_ARCH_SH4
/** Intel 386 */
#define CE_CAB_000_ARCH_I386 386
#define CE_CAB_000_ARCH_I386_NAME CE_ARCH_X86
/** Intel 486 */
#define CE_CAB_000_ARCH_I486 486
#define CE_CAB_000_ARCH_I486_NAME CE_ARCH_X86
/** Intel Pentium */
#define CE_CAB_000_ARCH_I586 586
#define CE_CAB_000_ARCH_I586_NAME CE_ARCH_X86
/** PowerPC 601 */
#define CE_CAB_000_ARCH_PPC601 601
#define CE_CAB_000_ARCH_PPC601_NAME "UNKNOWN"
/** PowerPC 603 */
#define CE_CAB_000_ARCH_PPC603 603
#define CE_CAB_000_ARCH_PPC603_NAME "UNKNOWN"
/** PowerPC 604 */
#define CE_CAB_000_ARCH_PPC604 604
#define CE_CAB_000_ARCH_PPC604_NAME "UNKNOWN"
/** PowerPC 620 */
#define CE_CAB_000_ARCH_PPC620 620
#define CE_CAB_000_ARCH_PPC620_NAME "UNKNOWN"
/** Motorola 821 */
#define CE_CAB_000_ARCH_MOTOROLA_821 821
#define CE_CAB_000_ARCH_MOTOROLA_821_NAME "UNKNOWN"
/** ARM 720 */
#define CE_CAB_000_ARCH_ARM720 1824
#define CE_CAB_000_ARCH_ARM720_NAME CE_ARCH_ARM
/** ARM 820 */
#define CE_CAB_000_ARCH_ARM820 2080
#define CE_CAB_000_ARCH_ARM820_NAME CE_ARCH_ARM
/** ARM 920 */
#define CE_CAB_000_ARCH_ARM920 2336
#define CE_CAB_000_ARCH_ARM920_NAME CE_ARCH_ARM
/** StrongARM */
#define CE_CAB_000_ARCH_STRONGARM 2577
#define CE_CAB_000_ARCH_STRONGARM_NAME CE_ARCH_ARM
/** MIPS R4000 */
#define CE_CAB_000_ARCH_R4000 4000
#define CE_CAB_000_ARCH_R4000_NAME CE_ARCH_MIPS
/** Hitachi SH3 */
#define CE_CAB_000_ARCH_HITACHI_SH3 10003
#define CE_CAB_000_ARCH_HITACHI_SH3_NAME CE_ARCH_SH3
/** Hitachi SH3E */
#define CE_CAB_000_ARCH_HITACHI_SH3E 10004
#define CE_CAB_000_ARCH_HITACHI_SH3E_NAME CE_ARCH_SH3
/** Hitachi SH4 */
#define CE_CAB_000_ARCH_HITACHI_SH4 10005
#define CE_CAB_000_ARCH_HITACHI_SH4_NAME CE_ARCH_SH4
/** Alpha 21064 */
#define CE_CAB_000_ARCH_ALPHA 21064
#define CE_CAB_000_ARCH_ALPHA_NAME "UNKNOWN"
/** ARM 7TDMI */
#define CE_CAB_000_ARCH_ARM7TDMI 70001
#define CE_CAB_000_ARCH_ARM7TDMI_NAME CE_ARCH_ARM
