#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifndef __MINGW32__
#include <sys/mman.h>
#endif

#include "WinCEArchitecture.h"
#include "WinCECab000Header.h"
#include "cjson/cJSON.h"

#define PROGRAM_NAME "wcecabinfo"
#define PROGRAM_VERSION "0.9.0"

struct opts {
    /** Print output as JSON */
    bool printJson : 1;
    /** Print output as Windows REG format */
    bool printReg : 1;
    /** Print verbose information */
    bool verbose : 1;
    /** Filter field */
    const char *filterField;
    /** Input file path */
    const char *infile;
};

typedef struct infile_struct {
    const void *file;
    size_t size;
} infile_struct;

/** Pointer to the (possibily memory-mapped) file contents */
static void *file;
/** CAB Header structure */
static const CE_CAB_000_HEADER *cabheader;

/**
 * @brief Print usage and exit program
 *
 * @param status exit status
 */
void usage(int status) {
    puts(
        "Usage: " PROGRAM_NAME
        " [-j] [-r] [-V] FILE"
        "\n"
        "Print information about a CAB .000 file. Input can be either a cab file or an already extracted .000 file.\n"
        "If a cab file is provided, cabextract is needed to handle extraction.\n"
        "\n"
        "  -j, --json               print output as JSON\n"
        "  -r, --reg                print output as Windows Reg format\n"
        //"  -f, --field FIELDNAME    only print the value of the field with key FIELDNAME\n"
        "                           overrides --json option\n"
        "  -h, --help               print help\n"
        "  -v, --version            print version information\n"
        "  -V, --verbose            print verbose logs\n"
        "\n"
        "Examples:\n"
        "  " PROGRAM_NAME
        " f.cab     Print information about file f.cab\n"
        "  " PROGRAM_NAME " -j f.000  Print JSON formatted information about file f.000");
    exit(status);
}

/**
 * @brief Print version
 */
void version() {
    puts(PROGRAM_NAME);
    puts("Version " PROGRAM_VERSION);
    exit(EXIT_SUCCESS);
}

/**
 * @brief Get the commandline options
 *
 * @param argc
 * @param argv
 * @return struct opts*
 */
static struct opts options;
static inline struct opts *get_opts(int argc, char **argv) {
    opterr = 0;

    char c;

    static struct option long_options[] = {{"json", no_argument, 0, 'j'},
                                           {"reg", no_argument, 0, 'r'},
                                           {"help", no_argument, NULL, 'h'},
                                           {"version", no_argument, NULL, 'v'},
                                           {"verbose", no_argument, NULL, 'V'},
                                           {"field", required_argument, NULL, 'f'},
                                           {NULL, 0, NULL, 0}};
    /** getopt_long stores the option index here. */
    int option_index = 0;

    while ((c = getopt_long(argc, argv, "jrbhvVf:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'j':
                options.printJson = true;
                break;
            case 'r':
                options.printReg = true;
                break;
            case 'h':
                usage(0);
                break;
            case 'f':
                options.filterField = optarg;
                break;
            case 'v':
                version();
                break;
            case 'V':
                options.verbose = true;
                break;
            default:
                abort();
        }
    }

    if (options.printJson && options.printReg) {
        fprintf(stderr, "Error: --json and --reg are mutually exclusive\n");
        exit(EXIT_FAILURE);
    }

    /* field option overrides json option */
    if (options.filterField) {
        options.printJson = 0;
    }

    options.infile = "-";

    if (optind < argc) {
        if (isatty(STDIN_FILENO)) {
            options.infile = argv[optind++];
        } else {
            fprintf(stderr, "Input file argument provided while not in a TTY");
            exit(EXIT_FAILURE);
        }
    } else if (isatty(STDIN_FILENO)) {
        usage(0);
    }

    return &options;
}

/**
 * @brief Check whether a string ends with the provided string
 *
 * @param input string to test
 * @param tail string to check for
 * @return int 1 if string ends with tail, 0 if not
 */
int strendswith(const char *input, const char *tail) {
    const size_t input_strlen = strlen(input);
    const size_t tail_strlen = strlen(tail);
    if (tail_strlen > input_strlen) {
        return 0;
    }
    return strcmp(input + input_strlen - tail_strlen, tail) == 0;
}

#ifndef __MINGW32__
/**
 * @brief Get the contents of the 000 file and return the pointer to it
 *
 * @param file_path
 * @return void*
 */
int read000filecontents(const char *file_path, infile_struct *file_info) {
    struct stat s;
    int status;
    const void *mapped;
    int i;

    /* try to open file */
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open %s failed: %s", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Get the size of the file. */
    status = fstat(fd, &s);
    if (status < 0) {
        fprintf(stderr, "fstat %s failed: %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    file_info->size = s.st_size;

    /* Memory-map the file. */
    mapped = mmap(0, file_info->size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        fprintf(stderr, "mmap %s failed: %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    close(fd);

    file_info->file = mapped;
    return 1;
}
#endif

/**
 * @brief Checks if a file has a 32 bit header
 *
 * @param file_path File path
 * @return uint8_t 1 is the file is a cab file, 0 if not
 */
uint8_t filehasheader(const char *file_path, const uint32_t header_signature) {
    // fprintf(stdout, "Opening file '%s'.\n", file_path);
    /* try to open file */
    FILE *fp = fopen(file_path, "rb");

    if (!fp) {
        fprintf(stderr, "error: file open failed '%s'.\n", file_path);
        exit(EXIT_FAILURE);
    }

    uint32_t c;
    fread(&c, sizeof(c), 1, fp);
    fclose(fp);
    return c == header_signature;
}

#define CHUNK_SIZE 1024

/**
 * @brief Read 000 file from an input stream
 *
 * @param stream stream to read from
 * @param file_info struct to write file handle and size into
 * @return uint8_t*
 */
uint8_t read000filestream(FILE *stream, infile_struct *file_info) {
    void *buffer = malloc(CHUNK_SIZE);

    size_t c = 0;
    size_t file_size = 0;

    uint16_t chunks = 1;

    while (c = fread(buffer + file_size, 1, CHUNK_SIZE, stream)) {
        char *old = buffer;

        file_size += c;
        buffer = realloc(buffer, CHUNK_SIZE * (++chunks));
        if (buffer == NULL) {
            perror("Failed to reallocate content");
            free(old);
            exit(EXIT_FAILURE);
        }
    }

    if (ferror(stream)) {
        perror("Error while reading from stream");
        exit(EXIT_FAILURE);
    }

    file_info->size = file_size;
    file_info->file = buffer;

    return 1;
}

static bool verbose_enabled = false;

/**
 * @brief Print verbose message
 *
 * @param format Format string
 * @param ... varargs
 * @return int return code of vprintf
 */
static int verbose(const char *restrict format, ...) {
    if (!verbose_enabled) return 0;

    va_list args;
    va_start(args, format);
    int ret = vfprintf(stderr, format, args);
    va_end(args);

    return ret;
}

const char **get_unsupported(const char *usup, uint16_t len) {
    uint16_t numUnsupported = 1;

    for (char *ptr = (char *)usup; ptr < (char *)(usup + len); ptr++) {
        if (*ptr == '\0') {
            numUnsupported++;
        }
    }

    const char **unsupported = malloc((numUnsupported + 1) * sizeof(size_t));

    uint16_t idx = 0;
    char prevValue = '\0';
    for (char *ptr = (char *)usup; ptr < (char *)(usup + len); ptr++) {
        if (prevValue == '\0') {
            unsupported[idx++] = ptr;
        }
        prevValue = *ptr;
    }

    unsupported[numUnsupported - 1] = NULL;

    return unsupported;
}

/**
 * @brief Get the corresponding strong for the hive id in the Reg Hives section
 * of the 000 header
 *
 * @param hiveid Hive id (1-4)
 * @return const char* Returns sorresponding string, or NULL of the ID is
 * invalid
 */
const char *get_hive(uint16_t hiveid) {
    switch (hiveid) {
        case 1:
            return "HKEY_CLASSES_ROOT";
        case 2:
            return "HKEY_CURRENT_USER";
        case 3:
            return "HKEY_LOCAL_MACHINE";
        case 4:
            return "HKEY_USERS";
        default:
            return NULL;
    }
}

/**
 * @brief Get the string with the specified id from the strings section of the
 * cab 000 file
 *
 * @param stringid String id to fetch
 * @return const char* pointer to the string, or 000 if string does not exist.
 */
const char *get_string(uint16_t stringid) {
    // verbose("getString(%d) = ", stringid);

    CE_CAB_000_STRING_ENTRY *stringentry = (CE_CAB_000_STRING_ENTRY *)(file + cabheader->OffsetStrings);
    for (int i = 0; i < cabheader->NumEntriesString; i++) {
        if (stringentry->Id == stringid) {
            // verbose("\"%s\" (length: %d)\n", &(stringentry->String), stringentry->StringLength);

            return &(stringentry->String);
        }
        stringentry = ((void *)stringentry) + stringentry->StringLength + sizeof(uint16_t) + sizeof(uint16_t);
    }
    // verbose("NULL\n");
    return NULL;
}

/**
 * @brief Parse a spec array which contains a list of 16-bit String IDs,
 * terminated by 0
 *
 * @param spec pointer to spec array
 * @return const char* parsed String
 */
const char *parse_spec(uint16_t *spec, uint16_t speclength, char *delimiter) {
    char *buf = (char *)malloc(256);
    strcpy(buf, "");

    for (int i = 0; i < (speclength / sizeof(uint16_t) - 1); i++) {
        // verbose("spec[%d] = %d\n", i, spec[i]);
        if (i) {
            strcat(buf, delimiter);
        }
        strcat(buf, get_string(spec[i]));
        // verbose("buf=\"%s\"\n", buf);
    }

    return buf;
}

const char *get_basedir(uint16_t basedirid) {
    if (basedirid >= 0 && basedirid <= sizeof(BASE_DIRS)) {
        return BASE_DIRS[basedirid];
    }
    return NULL;
}

const char *get_dir(uint16_t directoryid) {
    CE_CAB_000_DIRECTORY_ENTRY *direntry = (CE_CAB_000_DIRECTORY_ENTRY *)(file + cabheader->OffsetDirs);
    for (int i = 0; i < cabheader->NumEntriesDirs; i++) {
        if (direntry->Id == directoryid) {
            return parse_spec(&(direntry->Spec), direntry->SpecLength, "\\");
        }
        direntry = ((void *)direntry) + direntry->SpecLength + sizeof(uint16_t) + sizeof(uint16_t);
    }
    return NULL;
}

const char *get_file(uint16_t fileid) {
    CE_CAB_000_FILE_ENTRY *fileentry = (CE_CAB_000_FILE_ENTRY *)(file + cabheader->OffsetFiles);
    for (int i = 0; i < cabheader->NumEntriesFiles; i++) {
        if (fileentry->Id == fileid) {
            return &(fileentry->FileName);
        }
        fileentry = ((void *)fileentry) + fileentry->FileNameLength + sizeof(CE_CAB_000_FILE_ENTRY) - 2;
    }
    return NULL;
}

const char *get_architecture(uint32_t archid) {
    switch (archid) {
        case CE_CAB_000_ARCH_SH3:
            return CE_ARCH_SH3;  // SHx SH3
        case CE_CAB_000_ARCH_SH4:
            return CE_ARCH_SH4;  // SHx SH4
        case CE_CAB_000_ARCH_I386:
            return CE_ARCH_X86;  // Intel 386
        case CE_CAB_000_ARCH_I486:
            return CE_ARCH_X86;  // Intel 486
        case CE_CAB_000_ARCH_I586:
            return CE_ARCH_X86;  // Intel Pentium
        case CE_CAB_000_ARCH_PPC601:
            return "PPC601";  // PowerPC 601
        case CE_CAB_000_ARCH_PPC603:
            return "PPC602";  // PowerPC 603
        case CE_CAB_000_ARCH_PPC604:
            return "PPC604";  // PowerPC 604
        case CE_CAB_000_ARCH_PPC620:
            return "PPC620";  // PowerPC 620
        case CE_CAB_000_ARCH_MOTOROLA_821:
            return "MOTOROLA821";  // Motorola 821
        case CE_CAB_000_ARCH_ARM720:
            return CE_ARCH_ARM;  // ARM 720
        case CE_CAB_000_ARCH_ARM820:
            return CE_ARCH_ARM;  // ARM 820
        case CE_CAB_000_ARCH_ARM920:
            return CE_ARCH_ARM;  // ARM 920
        case CE_CAB_000_ARCH_STRONGARM:
            return CE_ARCH_ARM;  // StrongARM
        case CE_CAB_000_ARCH_R4000:
            return CE_ARCH_MIPS;  // MIPS R4000
        case CE_CAB_000_ARCH_HITACHI_SH3:
            return CE_ARCH_SH3;  // Hitachi SH3
        case CE_CAB_000_ARCH_HITACHI_SH3E:
            return CE_ARCH_SH3;  // Hitachi SH3E
        case CE_CAB_000_ARCH_HITACHI_SH4:
            return CE_ARCH_SH4;  // Hitachi SH4
        case CE_CAB_000_ARCH_ALPHA:
            return "ALPHA";  // Alpha 21064
        case CE_CAB_000_ARCH_ARM7TDMI:
            return CE_ARCH_THUMB;  // ARM 7TDMI
        default:
            return NULL;
    }
}

const char *get_reg_datatype(uint32_t flags) {
    // printf("FLAGS: 0x%08x", flags);
    uint32_t masked = flags & 0x00010001;
    switch (masked) {
        case TYPE_REG_DWORD:
            return "REG_DWORD";
        case TYPE_REG_SZ:
            return "REG_SZ";
        case TYPE_REG_MULTI_SZ:
            return "REG_MULTI_SZ";
        case TYPE_REG_BINARY:
            return "REG_BINARY";
    }
}

const char *concat_paths(const char *path1, const char *path2) {
    uint16_t len = strlen(path1) + strlen(path2) + 2;
    char *out = malloc(len);
    sprintf(out, "%s\\%s", path1, path2);
    return out;
}

const char *get_file_full_path(uint16_t fileid) {
    CE_CAB_000_FILE_ENTRY *fileentry = (CE_CAB_000_FILE_ENTRY *)(file + cabheader->OffsetFiles);
    for (int i = 0; i < cabheader->NumEntriesFiles; i++) {
        if (fileid == fileentry->Id) {
            return concat_paths(get_dir(fileentry->DirectoryId), &(fileentry->FileName));
        }
        fileentry = ((void *)fileentry) + fileentry->FileNameLength + sizeof(CE_CAB_000_FILE_ENTRY) - sizeof(uint16_t);
    }
}

const char *get_reg_path(uint16_t hiveid) {
    CE_CAB_000_REGHIVE_ENTRY *reghiveentry = (CE_CAB_000_REGHIVE_ENTRY *)(file + cabheader->OffsetRegHives);
    for (int i = 0; i < cabheader->NumEntriesRegHives; i++) {
        uint16_t id = reghiveentry->Id;
        if (id == hiveid) return concat_paths(get_hive(reghiveentry->HiveRoot), parse_spec(&(reghiveentry->Spec), reghiveentry->SpecLength, "\\"));
        reghiveentry = ((void *)reghiveentry) + reghiveentry->SpecLength + sizeof(CE_CAB_000_REGHIVE_ENTRY) - sizeof(uint16_t);
    }
    return NULL;
}

int main(int argc, char **argv) {
    /** commandline options */
    struct opts *options = get_opts(argc, argv);
    size_t file_size;
    infile_struct file_info;
    verbose_enabled = options->verbose;

    if (isatty(STDIN_FILENO)) {
        const char *ext = strrchr(options->infile, '.');
        if (access(options->infile, R_OK) == -1) {
            fprintf(stderr, "Error: File can not be read or does not exist.");
            exit(EXIT_FAILURE);
        }

        // Application is running in a TTY
        if (filehasheader(options->infile, CE_CAB_HEADER_SIGNATURE)) {
            verbose("File was identified as a CAB file by file signature\n");
            // Check file extension
            if (!strcasecmp(ext, "CAB")) {
                fprintf(stderr, "Warning: File appears to be a CAB file, but does not have a .cab extension");
            }
            // check if cabextract is available
            if (system("which cabextract > /dev/null 2>&1")) {
                fprintf(stderr, "cabextract not found. Please install this dependency.\nhttps://www.cabextract.org.uk/\n");
                exit(EXIT_FAILURE);
            }
            // Use cabextract to extract the 000 file and read the pipe
            char *cabextractcmd = malloc(256 + strlen(options->infile));
            sprintf(cabextractcmd, "cabextract --pipe --filter *.000 %s", options->infile);
            FILE *pcabextract = popen(cabextractcmd, "r");

            // Read cabextract output
            read000filestream(pcabextract, &file_info);

            // Check if cabextract succeeded
            int status = pclose(pcabextract);
            verbose("cabextract exited with status %d\n", status);
            if (status) {
                fprintf(stderr, "Error: cabextract existed with status %d\n", status);
                exit(EXIT_FAILURE);
            }
        } else if (filehasheader(options->infile, CE_CAB_000_HEADER_SIGNATURE)) {
            verbose("File was identified as a 000 file by file signature\n");

            // Check file extension
            if (!strcasecmp(ext, "000")) {
                fprintf(stderr,
                        "Warning: File appears to be a 000 file, but does not have "
                        "a .000 extension");
            }

// Read file contents of the 000 file
#ifndef __MINGW32__
            read000filecontents(options->infile, &file_info);
#else
            read000filestream(fopen(options->infile, "r"), &file_info);
#endif
        } else {
            fprintf(stderr, "Error: Input file is neither a CAB file nor a 000 file\n");
            exit(EXIT_FAILURE);
        }
    } else {
        // Application is not running in a TTY, assume that the file is being
        // piped in

        // CAB file
        // fprintf(stderr,("Piping in CAB files is supported at this time\n");
        // exit(EXIT_FAILURE);

        // TODO make difference between cab file and 000 file

        read000filestream(stdin, &file_info);
    }

    file = (void *)file_info.file;
    file_size = (size_t)file_info.size;

    verbose("Opened file, size: %d\n", file_size);

    if (!file_size) {
        fprintf(stderr, "Error: Input size is 0\n");
        exit(EXIT_FAILURE);
    }

    cabheader = (CE_CAB_000_HEADER *)file;

    if (cabheader->AsciiSignature != CE_CAB_000_HEADER_SIGNATURE) {
        fprintf(stderr, "Error: Input file is not a .000 file\n");
        exit(EXIT_FAILURE);
    }

    if (cabheader->FileLength != file_size) {
        fprintf(stderr, "Error: 000 header file length (%d) and actual file length (%d) don't match\n", cabheader->FileLength, (uint32_t)file_size);
        exit(EXIT_FAILURE);
    }

    verbose("AsciiSignature: %#08X\n", cabheader->AsciiSignature);
    verbose("Unknown1: %d\n", cabheader->Unknown1);
    verbose("FileLength: %d\n", cabheader->FileLength);
    verbose("Unknown2: %d\n", cabheader->Unknown2);
    verbose("Unknown3: %d\n", cabheader->Unknown3);
    verbose("TargetArchitecture: %d\n", cabheader->TargetArchitecture);
    verbose("MinCEVersionMajor: %d\n", cabheader->MinCEVersionMajor);
    verbose("MinCEVersionMinor: %d\n", cabheader->MinCEVersionMinor);
    verbose("MaxCEVersionMajor: %d\n", cabheader->MaxCEVersionMajor);
    verbose("MaxCEVersionMinor: %d\n", cabheader->MaxCEVersionMinor);
    verbose("MinCEBuildNumber: %d\n", cabheader->MinCEBuildNumber);
    verbose("MaxCEBuildNumber: %d\n", cabheader->MaxCEBuildNumber);
    verbose("NumEntriesString: %d\n", cabheader->NumEntriesString);
    verbose("NumEntriesDirs: %d\n", cabheader->NumEntriesDirs);
    verbose("NumEntriesFiles: %d\n", cabheader->NumEntriesFiles);
    verbose("NumEntriesRegHives: %d\n", cabheader->NumEntriesRegHives);
    verbose("NumEntriesRegKeys: %d\n", cabheader->NumEntriesRegKeys);
    verbose("NumEntriesLinks: %d\n", cabheader->NumEntriesLinks);
    verbose("OffsetStrings: %d\n", cabheader->OffsetStrings);
    verbose("OffsetDirs: %d\n", cabheader->OffsetDirs);
    verbose("OffsetFiles: %d\n", cabheader->OffsetFiles);
    verbose("OffsetRegHives: %d\n", cabheader->OffsetRegHives);
    verbose("OffsetRegKeys: %d\n", cabheader->OffsetRegKeys);
    verbose("OffsetLinks: %d\n", cabheader->OffsetLinks);
    verbose("OffsetAppname: %d\n", cabheader->OffsetAppname);
    verbose("LengthAppname: %d\n", cabheader->LengthAppname);
    verbose("OffsetProvider: %d\n", cabheader->OffsetProvider);
    verbose("LengthProvider: %d\n", cabheader->LengthProvider);
    verbose("OffsetUnsupported: %d\n", cabheader->OffsetUnsupported);
    verbose("LengthUnsupported: %d\n", cabheader->LengthUnsupported);
    verbose("Unknown4: %d\n", cabheader->Unknown4);
    verbose("Unknown5: %d\n", cabheader->Unknown5);

    const char *appName = (const char *)(file + cabheader->OffsetAppname);
    const char *provider = (const char *)(file + cabheader->OffsetProvider);
    const char *architecture = get_architecture(cabheader->TargetArchitecture);

    const char *usup = (const char *)(file + cabheader->OffsetUnsupported);
    const uint16_t usuplen = cabheader->LengthUnsupported;
    const char **unsupported = get_unsupported(usup, usuplen);

    if (options->printJson) {
        char *buffer = malloc(256);

        /** Root JSON Object */
        cJSON *cabJson = cJSON_CreateObject();

        /** App Name JSON Object */
        cJSON *appNameJson = cJSON_CreateString(appName);
        cJSON_AddItemToObject(cabJson, "appName", appNameJson);

        /** Provider JSON Object */
        cJSON *providerJson = cJSON_CreateString(provider);
        cJSON_AddItemToObject(cabJson, "provider", providerJson);

        /** Provider JSON Object */
        if (architecture) {
            cJSON *architectureJson = cJSON_CreateString(architecture);
            cJSON_AddItemToObject(cabJson, "architecture", architectureJson);
        } else {
            cJSON_AddItemToObject(cabJson, "architecture", cJSON_CreateNull());
        }

        /** Unsupported */
        if (*unsupported) {
            cJSON *unsupportedJson = cJSON_CreateArray();
            for (int i = 0; unsupported[i] && strlen(unsupported[i]); i++) {
                cJSON_AddItemToArray(unsupportedJson, cJSON_CreateString(unsupported[i]));
            }
            cJSON_AddItemToObject(cabJson, "unsupported", unsupportedJson);
        }

        /** Min CE Version */
        if (cabheader->MinCEVersionMajor) {
            cJSON *minCeVersionJson = cJSON_CreateObject();
            /** Min CE Version Major JSON Object */
            cJSON *minCeVersionMajorJson = cJSON_CreateNumber(cabheader->MinCEVersionMajor);
            cJSON_AddItemToObject(minCeVersionJson, "major", minCeVersionMajorJson);
            cJSON *minCeVersionMinorJson = cJSON_CreateNumber(cabheader->MinCEVersionMinor);
            cJSON_AddItemToObject(minCeVersionJson, "minor", minCeVersionMinorJson);
            /** Min CE Version String */
            sprintf(buffer, "%d.%d", cabheader->MinCEVersionMajor, cabheader->MinCEVersionMinor);
            cJSON *minCeVersionStringJson = cJSON_CreateString(buffer);
            cJSON_AddItemToObject(minCeVersionJson, "stringValue", minCeVersionStringJson);
            cJSON_AddItemToObject(cabJson, "minCeVersion", minCeVersionJson);
        }

        /** Max CE Version */
        if (cabheader->MaxCEVersionMajor) {
            cJSON *maxCeVersionJson = cJSON_CreateObject();
            /** Min CE Version Major JSON Object */
            cJSON *maxCeVersionMajorJson = cJSON_CreateNumber(cabheader->MaxCEVersionMajor);
            cJSON_AddItemToObject(maxCeVersionJson, "major", maxCeVersionMajorJson);
            cJSON *maxCeVersionMinorJson = cJSON_CreateNumber(cabheader->MaxCEVersionMinor);
            cJSON_AddItemToObject(maxCeVersionJson, "minor", maxCeVersionMinorJson);
            /** Min CE Version String */
            sprintf(buffer, "%d.%d", cabheader->MaxCEVersionMajor, cabheader->MaxCEVersionMinor);
            cJSON *maxCeVersionStringJson = cJSON_CreateString(buffer);
            cJSON_AddItemToObject(maxCeVersionJson, "stringValue", maxCeVersionStringJson);
            cJSON_AddItemToObject(cabJson, "maxCeVersion", maxCeVersionJson);
        }

        /** Min CE build number */
        if (cabheader->MinCEBuildNumber) {
            cJSON *minCeBuildNumberJson = cJSON_CreateNumber(cabheader->MinCEBuildNumber);
            cJSON_AddItemToObject(cabJson, "minCeBuildNumber", minCeBuildNumberJson);
        }

        /** Max CE build number */
        if (cabheader->MaxCEBuildNumber) {
            cJSON *maxCeBuildNumberJson = cJSON_CreateNumber(cabheader->MaxCEBuildNumber);
            cJSON_AddItemToObject(cabJson, "maxCeBuildNumber", maxCeBuildNumberJson);
        }

        /** Directories */
        cJSON *directoriesJson = cJSON_CreateArray();
        CE_CAB_000_DIRECTORY_ENTRY *directoryentry = (CE_CAB_000_DIRECTORY_ENTRY *)(file + cabheader->OffsetDirs);
        for (int i = 0; i < cabheader->NumEntriesDirs; i++) {
            cJSON *directoryItem = cJSON_CreateObject();

            // cJSON *specLength = cJSON_CreateNumber(directoryentry->SpecLength);
            // cJSON_AddItemToObject(directoryItem, "specLength", specLength);

            /** Directory ID */
            cJSON *directoryId = cJSON_CreateNumber(directoryentry->Id);
            cJSON_AddItemToObject(directoryItem, "id", directoryId);

            /** Directory Path */
            cJSON *directoryPath = cJSON_CreateString(parse_spec(&(directoryentry->Spec), directoryentry->SpecLength, "\\"));
            cJSON_AddItemToObject(directoryItem, "path", directoryPath);

            directoryentry = ((void *)directoryentry) + directoryentry->SpecLength + sizeof(CE_CAB_000_DIRECTORY_ENTRY) - sizeof(uint16_t);
            cJSON_AddItemToArray(directoriesJson, directoryItem);
        }
        cJSON_AddItemToObject(cabJson, "directories", directoriesJson);

        /** Files */
        cJSON *filesJson = cJSON_CreateArray();
        CE_CAB_000_FILE_ENTRY *fileentry = (CE_CAB_000_FILE_ENTRY *)(file + cabheader->OffsetFiles);
        for (int i = 0; i < cabheader->NumEntriesFiles; i++) {
            cJSON *fileItem = cJSON_CreateObject();

            /** File ID */
            cJSON *fileId = cJSON_CreateNumber(fileentry->Id);
            cJSON_AddItemToObject(fileItem, "id", fileId);

            /** File Name */
            cJSON *fileName = cJSON_CreateString(&(fileentry->FileName));
            cJSON_AddItemToObject(fileItem, "name", fileName);

            /** File Directory */
            cJSON *directory = cJSON_CreateString(get_dir(fileentry->DirectoryId));
            cJSON_AddItemToObject(fileItem, "directory", directory);

            // Flags
            if (fileentry->FlagsUpper & 0x8000) {
                cJSON_AddItemToObject(fileItem, "isReferenceCountingSharedFile", cJSON_CreateTrue());
            }
            if (fileentry->FlagsUpper & 0x4000) {
                cJSON_AddItemToObject(fileItem, "ignoreCabFileDate", cJSON_CreateTrue());
            }
            if (fileentry->FlagsUpper & 0x2000) {
                cJSON_AddItemToObject(fileItem, "doNotOverWriteIfTargetIsNewer", cJSON_CreateTrue());
            }
            if (fileentry->FlagsUpper & 0x1000) {
                cJSON_AddItemToObject(fileItem, "selfRegisterDll", cJSON_CreateTrue());
            }
            if (fileentry->FlagsLower & 0x0400) {
                cJSON_AddItemToObject(fileItem, "doNotCopyUnlessTargetExists", cJSON_CreateTrue());
            }
            if (fileentry->FlagsLower & 0x0010) {
                cJSON_AddItemToObject(fileItem, "overWriteTargetIfExists", cJSON_CreateTrue());
            }
            if (fileentry->FlagsLower & 0x0002) {
                cJSON_AddItemToObject(fileItem, "doNotSkip", cJSON_CreateTrue());
            }
            if (fileentry->FlagsLower & 0x0001) {
                cJSON_AddItemToObject(fileItem, "warnIfSkipped", cJSON_CreateTrue());
            }

            fileentry = ((void *)fileentry) + fileentry->FileNameLength + sizeof(CE_CAB_000_FILE_ENTRY) - 2;
            cJSON_AddItemToArray(filesJson, fileItem);
        }
        cJSON_AddItemToObject(cabJson, "files", filesJson);

        /** RegistryEntries */
        cJSON *registryEntriesJson = cJSON_CreateArray();
        CE_CAB_000_REGKEY_ENTRY *regkeyentry = (CE_CAB_000_REGKEY_ENTRY *)(file + cabheader->OffsetRegKeys);
        for (int i = 0; i < cabheader->NumEntriesRegKeys; i++) {
            const char *name = &(regkeyentry->KeyName);
            const char *value = &(regkeyentry->KeyName) + strlen(&(regkeyentry->KeyName)) + 1;
            const char *datatype = get_reg_datatype(regkeyentry->TypeFlagsUpper << 16 | regkeyentry->TypeFlagsLower);
            const uint16_t datalength = regkeyentry->DataLength - strlen(&(regkeyentry->KeyName)) - 1;
            uint32_t regtype = (regkeyentry->TypeFlagsUpper << 16 | regkeyentry->TypeFlagsLower) & TYPE_REG_MASK;
            const char *path = get_reg_path(regkeyentry->HiveId);
            uint8_t *ptr = (uint8_t *)value;

            /** Reg Key Item */
            cJSON *regKeyItem = cJSON_CreateObject();

            /** Reg path */
            cJSON *pathJson = cJSON_CreateString(path);
            cJSON_AddItemToObject(regKeyItem, "path", pathJson);

            /** Reg Item name. Null if default */
            cJSON *nameJson = strlen(name) ? cJSON_CreateString(name) : cJSON_CreateNull();
            cJSON_AddItemToObject(regKeyItem, "name", nameJson);

            /** Reg value data type */
            cJSON *dataTypeJson = cJSON_CreateString(datatype);
            cJSON_AddItemToObject(regKeyItem, "dataType", dataTypeJson);

            /** Reg item data */
            cJSON *valueJson;
            char *val;
            switch (regtype) {
                case TYPE_REG_DWORD:
                    val = (char *)malloc(16);
                    sprintf(val, "dword:%08X", *((uint32_t *)value));
                    break;
                case TYPE_REG_SZ:
                    val = (char *)value;
                    break;
                case TYPE_REG_MULTI_SZ:
                    val = malloc(8 + datalength * 3);
                    sprintf(val, "hex(7):");
                    for (uint16_t i = 0; i < datalength; i++) {
                        if (i) strcat(val, ",");
                        sprintf(val, "%02X", ptr[i]);
                    }
                    break;
                case TYPE_REG_BINARY:
                    val = malloc(8 + datalength * 3);
                    sprintf(val, "hex:");
                    for (uint16_t i = 0; i < datalength; i++) {
                        if (i) strcat(val, ",");
                        sprintf(val, "%02X", ptr[i]);
                    }
                    break;
            }
            valueJson = cJSON_CreateString(val);
            cJSON_AddItemToObject(regKeyItem, "value", valueJson);

            regkeyentry = ((void *)regkeyentry) + regkeyentry->DataLength + sizeof(CE_CAB_000_REGKEY_ENTRY) - sizeof(uint16_t);
            cJSON_AddItemToArray(registryEntriesJson, regKeyItem);
        }
        cJSON_AddItemToObject(cabJson, "registryEntries", registryEntriesJson);

        /** Links */
        cJSON *linksJson = cJSON_CreateArray();
        CE_CAB_000_LINK_ENTRY *linkentry = (CE_CAB_000_LINK_ENTRY *)(file + cabheader->OffsetLinks);
        for (int i = 0; i < cabheader->NumEntriesLinks; i++) {
            const char *basedir = get_basedir(linkentry->BaseDirectory);
            const char *linkspec = parse_spec(&(linkentry->Spec), linkentry->SpecLength + 2, "\\");

            cJSON *linkItem = cJSON_CreateObject();

            // cJSON *specLength = cJSON_CreateNumber(linkentry->SpecLength);
            // cJSON_AddItemToObject(linkItem, "specLength", specLength);

            // cJSON *linkId = cJSON_CreateNumber(linkentry->Id);
            // cJSON_AddItemToObject(linkItem, "linkId", linkId);

            cJSON *isFile = cJSON_CreateBool(linkentry->LinkType);
            cJSON_AddItemToObject(linkItem, "isFile", isFile);

            cJSON *targetId = cJSON_CreateNumber(linkentry->TargetId);
            cJSON_AddItemToObject(linkItem, "targetId", targetId);

            cJSON *linkPath = cJSON_CreateString(concat_paths(basedir, linkspec));
            cJSON_AddItemToObject(linkItem, "linkPath", linkPath);

            cJSON *targetPath;
            if (linkentry->LinkType) {
                targetPath = cJSON_CreateString(get_file_full_path(linkentry->TargetId));
            } else {
                targetPath = cJSON_CreateString(get_dir(linkentry->TargetId));
            }
            cJSON_AddItemToObject(linkItem, "targetPath", targetPath);

            linkentry = ((void *)linkentry) + linkentry->SpecLength + sizeof(CE_CAB_000_LINK_ENTRY) - sizeof(uint16_t);

            cJSON_AddItemToArray(linksJson, linkItem);
        }
        cJSON_AddItemToObject(cabJson, "links", linksJson);

        /** Stringified JSON Object */
        const char *stringJson = cJSON_Print(cabJson);
        if (stringJson == NULL) {
            fprintf(stderr, "Failed to print json.\n");
            exit(EXIT_FAILURE);
        }

        // Print JSON
        puts(stringJson);
    } else if (options->printReg) {
        // Reg file first line
        fprintf(stdout, "REGEDIT4\n");

        int previoushiveid = -1;

        CE_CAB_000_REGKEY_ENTRY *regkeyentry = (CE_CAB_000_REGKEY_ENTRY *)(file + cabheader->OffsetRegKeys);
        for (int i = 0; i < cabheader->NumEntriesRegKeys; i++) {
            const char *name = &(regkeyentry->KeyName);
            const uint16_t hiveid = regkeyentry->HiveId;
            const void *value = &(regkeyentry->KeyName) + strlen(&(regkeyentry->KeyName)) + 1;
            const char *datatype = get_reg_datatype(regkeyentry->TypeFlagsUpper << 16 | regkeyentry->TypeFlagsLower);
            const char *path = get_reg_path(regkeyentry->HiveId);
            const uint16_t datalength = regkeyentry->DataLength - strlen(&(regkeyentry->KeyName)) - 1;
            uint8_t *ptr = (uint8_t *)value;
            uint32_t regtype = (regkeyentry->TypeFlagsUpper << 16 | regkeyentry->TypeFlagsLower) & TYPE_REG_MASK;

            if (previoushiveid != hiveid) {
                fprintf(stdout, "\n[%s]\n", path);
            }
            // fprintf(stdout, "HideId: %d\n", regkeyentry->HiveId);
            // fprintf(stdout, "DataLength: %d\n", datalength);

            fprintf(stdout, strlen(name) ? "\"%s\"=" : "@=", name);
            switch (regtype) {
                case TYPE_REG_DWORD:
                    fprintf(stdout, "dword:%08X", *((uint32_t *)value));
                    break;
                case TYPE_REG_SZ:
                    fprintf(stdout, "\"%s\"", (char *)value);
                    break;
                case TYPE_REG_MULTI_SZ:
                    fprintf(stdout, "hex(7):");
                    for (uint16_t i = 0; i < datalength; i++) {
                        if (i) putc(',', stdout);
                        fprintf(stdout, "%02X", ptr[i]);
                    }
                    break;
                case TYPE_REG_BINARY:
                    fprintf(stdout, "hex:");
                    for (uint16_t i = 0; i < datalength; i++) {
                        if (i) putc(',', stdout);
                        fprintf(stdout, "%02X", ptr[i]);
                    }
                    break;
            }

            putc('\n', stdout);

            previoushiveid = hiveid;
            regkeyentry = ((void *)regkeyentry) + regkeyentry->DataLength + sizeof(CE_CAB_000_REGKEY_ENTRY) - sizeof(uint16_t);
        }

    } else {
        // Print output regularily

        printf("appName: %s\n", appName);
        printf("provider: %s\n", provider);

        if (architecture) {
            printf("architecture: %s\n", architecture);
        }

        if (*unsupported) {
            printf("unsupported: %s", unsupported[0]);
            for (int i = 1; unsupported[i] && strlen(unsupported[i]); i++) {
                printf(", %s", unsupported[i]);
            }
            putc('\n', stdout);
        }
        if (cabheader->MinCEVersionMajor) {
            printf("minCeVersion: %d.%d\n", cabheader->MinCEVersionMajor, cabheader->MinCEVersionMinor);
        }
        if (cabheader->MaxCEVersionMajor) {
            printf("maxCeVersion: %d.%d\n", cabheader->MaxCEVersionMajor, cabheader->MaxCEVersionMinor);
        }
        if (cabheader->MinCEBuildNumber) {
            printf("minCeBuildNumber: %d\n", cabheader->MinCEBuildNumber);
        }
        if (cabheader->MaxCEBuildNumber) {
            printf("maxCeBuildNumber: %d\n", cabheader->MaxCEBuildNumber);
        }
    }

#ifndef __MINGW32__
    // Unmap input file. Will do nothing if input file is not actually
    // memory mapped.
    munmap(file, file_size);
#endif
}
