#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "WinCECab000Header.h"
#include "cjson/cJSON.h"

struct opts {
    bool printJson : 1;
    bool onlyBasicInfo : 1;
    bool verbose : 1;
    const char *filterField;
    const char *infile;
};

#define PROGRAM_NAME "wcecabinfo"
#define PROGRAM_VERSION "0.1.1"

/**
 * @brief Print usage and exit program
 *
 * @param status exit status
 */
void usage(int status) {
    puts(
        "\
Usage: " PROGRAM_NAME
        " [-j] [-n] [-f FIELDNAME] FILE\
\n\
Print information about a CAB 000. Input can be either a cab file or an already extracted .000 file.\n\
If a cab file is provided, cabextract is needed to handle extraction.\n\
\n\
  -j, --json               print output as JSON\n\
  -f, --field FIELDNAME    only print the value of the field with key FIELDNAME\n\
                           overrides --json option\n\
  -h, --help               print help\n\
  -v, --version            print version information\n\
  -b, --basic              print only WCEApp, WCEArch and WCEVersion\n\
  -V, --verbose            print verbose logs\n\
\n\
Examples:\n\
  " PROGRAM_NAME
        " f.cab     Print information about file f.cab.\n\
  " PROGRAM_NAME
        " -j f.000  Print JSON formatted information about file f.000.");

    exit(status);
}

/**
 * @brief Print version
 */
void version() {
    puts("Version " PROGRAM_VERSION);
    exit(0);
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

    static struct option long_options[] = {
        {"json", no_argument, 0, 'j'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {"verbose", no_argument, NULL, 'V'},
        {"basic", no_argument, NULL, 'b'},
        {"field", required_argument, NULL, 'f'},
        {NULL, 0, NULL, 0}};
    /** getopt_long stores the option index here. */
    int option_index = 0;

    while ((c = getopt_long(argc, argv, "jbhvVf:", long_options,
                            &option_index)) != -1) {
        switch (c) {
            case 'j':
                options.printJson = true;
                break;
            case 'b':
                options.onlyBasicInfo = true;
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

    /* field option overrides json option */
    if (options.filterField || options.onlyBasicInfo) {
        options.printJson = 0;
    }

    if (options.filterField) {
        options.onlyBasicInfo = 0;
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

typedef struct infile_struct {
    const void *file;
    size_t size;
} infile_struct;

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

    while (c = fread(buffer + file_size, 1, CHUNK_SIZE, stream)) {
        char *old = buffer;

        file_size += c;
        buffer = realloc(buffer, CHUNK_SIZE);
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
    int ret = vprintf(format, args);
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

    const char **unsupported = malloc(numUnsupported);

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

int main(int argc, char **argv) {
    /** commandline options */
    struct opts *options = get_opts(argc, argv);
    void *file;
    size_t file_size;
    infile_struct file_info;
    verbose_enabled = options->verbose;

    const char *ext = strrchr(options->infile, '.');

    if (isatty(STDIN_FILENO)) {
        if (access(options->infile, R_OK) == -1) {
            fprintf(stderr, "Error: File can not be read or does not exist.");
            exit(EXIT_FAILURE);
        }

        // Application is running in a TTY
        if (filehasheader(options->infile, CE_CAB_HEADER_SIGNATURE)) {
            verbose("File was identified as a CAB file by file signature\n");
            // Check file extension
            if (!strcasecmp(ext, "CAB")) {
                fprintf(
                    stderr,
                    "Warning: File appears to be a CAB file, but does not have "
                    "a .cab extension");
            }
            // check if cabextract is available
            if (system("which cabextract > /dev/null 2>&1")) {
                fprintf(stderr,
                        "cabextract not found. Please install this dependency. "
                        "https://www.cabextract.org.uk/");
                exit(EXIT_FAILURE);
            }
            // Use cabextract to extract the 000 file and read the pipe
            char *cabextractcmd;
            sprintf(cabextractcmd, "cabextract --pipe --filter *.000 %s",
                    options->infile);
            FILE *pcabextract = popen("cabextract", "r");
            read000filestream(pcabextract, &file_info);

            int status = pclose(pcabextract);

            exit(EXIT_SUCCESS);
        } else if (filehasheader(options->infile,
                                 CE_CAB_000_HEADER_SIGNATURE)) {
            verbose("File was identified as a 000 file by file signature\n");

            // Check file extension
            if (!strcasecmp(ext, "000")) {
                fprintf(
                    stderr,
                    "Warning: File appears to be a 000 file, but does not have "
                    "a .000 extension");
            }

            // Read file contents of the 000 file
            read000filecontents(options->infile, &file_info);
        } else {
            fprintf(stderr,
                    "Error: Input file is neither a CAB file nor a 000 file\n");
            exit(EXIT_FAILURE);
        }
    } else {
        // Application is not running in a TTY, assume that the file is being
        // piped in

        // CAB file
        // fprintf(stderr,("Piping in CAB files is supported at this time\n");
        // exit(EXIT_FAILURE);

        read000filestream(stdin, &file_info);
    }
    file = (void *)file_info.file;
    file_size = (size_t)file_info.size;

    verbose("Opened file, size: %d\n", file_size);

    if (!file_size) {
        fprintf(stderr, "Input size is 0");
        exit(EXIT_FAILURE);
    }

    const CE_CAB_000_HEADER *cabheader = (CE_CAB_000_HEADER *)file;

    // printf("AsciiSignature: %#08X\n", cabheader->AsciiSignature);
    // printf("Unknown1: %d\n", cabheader->Unknown1);
    // printf("FileLength: %d\n", cabheader->FileLength);
    // printf("Unknown2: %d\n", cabheader->Unknown2);
    // printf("Unknown3: %d\n", cabheader->Unknown3);
    // printf("TargetArchitecture: %d\n", cabheader->TargetArchitecture);
    // printf("MinCEVersionMajor: %d\n", cabheader->MinCEVersionMajor);
    // printf("MinCEVersionMinor: %d\n", cabheader->MinCEVersionMinor);
    // printf("MaxCEVersionMajor: %d\n", cabheader->MaxCEVersionMajor);
    // printf("MaxCEVersionMinor: %d\n", cabheader->MaxCEVersionMinor);
    // printf("MinCEBuildNumber: %d\n", cabheader->MinCEBuildNumber);
    // printf("MaxCEBuildNumber: %d\n", cabheader->MaxCEBuildNumber);
    // printf("NumEntriesString: %d\n", cabheader->NumEntriesString);
    // printf("NumEntriesDirs: %d\n", cabheader->NumEntriesDirs);
    // printf("NumEntriesFiles: %d\n", cabheader->NumEntriesFiles);
    // printf("NumEntriesRegHives: %d\n", cabheader->NumEntriesRegHives);
    // printf("NumEntriesRegKeys: %d\n", cabheader->NumEntriesRegKeys);
    // printf("NumEntriesLinks: %d\n", cabheader->NumEntriesLinks);
    // printf("OffsetStrings: %d\n", cabheader->OffsetStrings);
    // printf("OffsetDirs: %d\n", cabheader->OffsetDirs);
    // printf("OffsetFiles: %d\n", cabheader->OffsetFiles);
    // printf("OffsetRegHives: %d\n", cabheader->OffsetRegHives);
    // printf("OffsetRegKeys: %d\n", cabheader->OffsetRegKeys);
    // printf("OffsetLinks: %d\n", cabheader->OffsetLinks);
    // printf("OffsetAppname: %d\n", cabheader->OffsetAppname);
    // printf("LengthAppname: %d\n", cabheader->LengthAppname);
    // printf("OffsetProvider: %d\n", cabheader->OffsetProvider);
    // printf("LengthProvider: %d\n", cabheader->LengthProvider);
    // printf("OffsetUnsupported: %d\n", cabheader->OffsetUnsupported);
    // printf("LengthUnsupported: %d\n", cabheader->LengthUnsupported);
    // printf("Unknown4: %d\n", cabheader->Unknown4);
    // printf("Unknown5: %d\n", cabheader->Unknown5);

    const char *appName = (const char *)(file + cabheader->OffsetAppname);
    const char *provider = (const char *)(file + cabheader->OffsetProvider);

    const char *usup = (const char *)(file + cabheader->OffsetUnsupported);
    const uint16_t usuplen = cabheader->LengthUnsupported;
    const char **unsupported = get_unsupported(usup, usuplen);

    if (options->printJson) {
        // Print output JSON formatted

        /** Root JSON Object */
        cJSON *cabJson = cJSON_CreateObject();

        /** App Name JSON Object */
        cJSON *appNameJson = cJSON_CreateString(appName);
        cJSON_AddItemToObject(cabJson, "appName", appNameJson);

        /** Provider JSON Object */
        cJSON *providerJson = cJSON_CreateString(provider);
        cJSON_AddItemToObject(cabJson, "provider", providerJson);

        /** Stringified JSON Object */
        const char *stringJson = cJSON_Print(cabJson);
        if (stringJson == NULL) {
            fprintf(stderr, "Failed to print monitor.\n");
            exit(EXIT_FAILURE);
        }

        // Print JSON
        puts(stringJson);
    } else {
        // Print output regularily

        printf("appName: %s\n", appName);
        printf("provider: %s\n", provider);

        if (*unsupported) {
            printf("unsupported: %s", unsupported[0]);
            for (int i = 1; unsupported[i]; i++) {
                printf(", %s", unsupported[i]);
            }
            putc('\n', stdout);
        }
    }

    // Unmap input file. Will do nothing if input file is not actually
    // memory mapped.
    munmap(file, file_size);
}
