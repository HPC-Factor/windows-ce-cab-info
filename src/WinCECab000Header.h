#include <stdint.h>

#include "WinCEArchitecture.h"

#define CE_CAB_000_HEADER_SIGNATURE 0x4543534D

#define CE_CAB_HEADER_SIGNATURE 0x4643534D

#define TYPE_REG_MASK 0x00010001
#define TYPE_REG_DWORD 0x00010001
#define TYPE_REG_SZ 0x00000000
#define TYPE_REG_MULTI_SZ 0x00010000
#define TYPE_REG_BINARY 0x00000001

const char* BASE_DIRS[] = {
    "%InstallDir%",
    "%CE1%",
    "%CE2%",
    "%CE3%",
    "%CE4%",
    "%CE5%",
    "%CE6%",
    "%CE7%",
    "%CE8%",
    "%CE9%",
    "%CE10%",
    "%CE11%",
    "%CE12%",
    "%CE13%",
    "%CE14%",
    "%CE15%",
    "%CE16%",
    "%CE17%",
};

typedef struct _CE_CAB_000_STRING_ENTRY {
    /** Integer string ID */
    uint16_t Id;
    /** The length of the string, including the null terminator byte */
    uint16_t StringLength;
    /* The string itself, for the length given above */
    char String;
} CE_CAB_000_STRING_ENTRY;

typedef struct _CE_CAB_000_DIRECTORY_ENTRY {
    /** Integer string ID */
    uint16_t Id;
    /** The length of the specification, including the null terminator bytes */
    uint16_t SpecLength;
    /* The string itself, for the length given above */
    uint16_t Spec;
} CE_CAB_000_DIRECTORY_ENTRY;

typedef struct _CE_CAB_000_REGHIVE_ENTRY {
    /** Integer hive ID */
    uint16_t Id;
    /** The hive root, an integer from 1 to 4:
     * HKCR: HKEY_CLASSES_ROOT
     * HKCU: HKEY_CURRENT_USER
     * HKLM: HKEY_LOCAL_MACHINE
     * HKU: HKEY_USERS
     */
    uint16_t HiveRoot;
    /** Unknown purpose, usually 0 */
    uint16_t Unknown;
    /** Length of the registry hive specification to follow, in bytes */
    uint16_t SpecLength;
    /* A registry hive specification. This is simply an array of 16-bit
     * (2 byte) numbers, where each number is a string ID. The final number is
     * always 0, which terminates the list */
    uint16_t Spec;
} CE_CAB_000_REGHIVE_ENTRY;

typedef struct _CE_CAB_000_FILE_ENTRY {
    /** An integer file ID. The file for installation is the file in the cabinet which has this integer as a three digit file extension */
    uint16_t Id;
    /** The directory ID for where this file should be installed */
    uint16_t DirectoryId;
    /** Unknown purpose, usually the same as the file ID */
    uint16_t Unknown;
    /** File flags. */
    uint16_t FlagsLower;
    uint16_t FlagsUpper;
    /** The length of the installed file's filename, including the null terminator */
    uint16_t FileNameLength;
    /** The installed file's filename, with the length given above. A null terminated ASCII string */
    char FileName;
} CE_CAB_000_FILE_ENTRY;

typedef struct _CE_CAB_000_REGKEY_ENTRY {
    /** Integer hive ID */
    uint16_t Id;
    /** The hive ID of the hive to store this entry in */
    uint16_t HiveId;
    /** Variable Substitution */
    uint16_t VariableSubstitution;
    /** The type of entry data, and flags */
    uint16_t TypeFlagsLower;
    uint16_t TypeFlagsUpper;
    /** Length of the registry key specification to follow, in bytes */
    uint16_t DataLength;
    /* Registry entry data. This begins with a null terminated ASCII
     * string, giving the name of the registry key. If the empty string is used
     * here, this is the default key for the given hive. Immediately following
     * the null byte of the string is the data for the registry entry */
    char KeyName;
} CE_CAB_000_REGKEY_ENTRY;

typedef struct _CE_CAB_000_LINK_ENTRY {
    /** Integer link ID */
    uint16_t Id;
    /** Unknown purpose */
    uint16_t Unknown;
    /** The base directory where the link will be stored, either 0 to indicate
     * "%InstallDir%", the default directory where the application is installed,
     * or a number from 1 to 17 to indicate "%CEn%", one of the standard
     * installation directories as defined in Appendix B. The link
     * specification, given below, should be added to this base directory, to
     * get the full name of the link */
    uint16_t BaseDirectory;
    /** Integer ID of the file or directory that the link should point to. It is
     * either a file ID, or a directory ID, depending on the link type. If it is
     * a directory ID, the ID 0 can be used to refer to %InstallDir% */
    uint16_t TargetId;
    /** The link type. This is 0 if the link target is a directory, or 1 if the
     * link target is a file */
    uint16_t LinkType;
    /** Length of the link specification to follow, in bytes */
    uint16_t SpecLength;
    /* The link specification. This is simply an array of 16-bit (2
     * byte) numbers, where each number is a string ID. The final number is
     * always 0, which terminates the list */
    uint16_t Spec;
} CE_CAB_000_LINK_ENTRY;

typedef struct _CE_CAB_000_HEADER {
    /** An ASCII signature, "MSCE". This is 0x4543534D as a little-endian
     * integer */
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
    /** Minimal version of WinCE (major version number) required to install this
     * cabinet, or 0 to indicate no restriction */
    uint32_t MinCEVersionMajor;
    /** Minimal version of WinCE (minor version number) required to install this
     * cabinet, or 0 to indicate no restriction */
    uint32_t MinCEVersionMinor;
    /** Maximal version of WinCE (major version number) required to install this
     * cabinet, or 0 to indicate no restriction */
    uint32_t MaxCEVersionMajor;
    /** Maximal version of WinCE (minor version number) required to install this
     * cabinet, or 0 to indicate no restriction */
    uint32_t MaxCEVersionMinor;
    /** Minmal version of WinCE (build number) required to install this cabinet,
     * or 0 to indicate no restriction */
    uint32_t MinCEBuildNumber;
    /** Maximal version of WinCE (build number) required to install this
     * cabinet, or 0 to indicate no restriction */
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
    /** The length of the APPNAME string in bytes, including null terminating
     * byte */
    uint16_t LengthAppname;
    /** The file offset of the PROVIDER string in bytes */
    uint16_t OffsetProvider;
    /** The length of the PROVIDER string in bytes, including null terminating
     * byte */
    uint16_t LengthProvider;
    /** The file offset of the UNSUPPORTED multi string in bytes */
    uint16_t OffsetUnsupported;
    /** The length of the UNSUPPORTED multi string in bytes, including null
     * terminating byte */
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
#define CE_CAB_000_ARCH_PPC601_NAME "PPC601"
/** PowerPC 603 */
#define CE_CAB_000_ARCH_PPC603 603
#define CE_CAB_000_ARCH_PPC603_NAME "PPC602"
/** PowerPC 604 */
#define CE_CAB_000_ARCH_PPC604 604
#define CE_CAB_000_ARCH_PPC604_NAME "PPC604"
/** PowerPC 620 */
#define CE_CAB_000_ARCH_PPC620 620
#define CE_CAB_000_ARCH_PPC620_NAME "PPC620"
/** Motorola 821 */
#define CE_CAB_000_ARCH_MOTOROLA_821 821
#define CE_CAB_000_ARCH_MOTOROLA_821_NAME "MOTOROLA821"
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
#define CE_CAB_000_ARCH_ALPHA_NAME "ALPHA"
/** ARM 7TDMI */
#define CE_CAB_000_ARCH_ARM7TDMI 70001
#define CE_CAB_000_ARCH_ARM7TDMI_NAME CE_ARCH_THUMB
