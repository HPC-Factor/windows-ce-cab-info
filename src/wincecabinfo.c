#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "cJSON.h"

struct opts {
    uint8_t printJson : 1;
    uint8_t onlyBasicInfo : 1;
    const char *filterField;
    const char *infile;
};

/**
 * @brief Get the commandline options
 *
 * @param argc
 * @param argv
 * @return struct opts*
 */
inline struct opts *get_opts(int argc, char **argv) {
    struct opts options;

    opterr = 0;

    char c;

    static struct option long_options[] = {
        {"json", no_argument, 0, 'j'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {"basic", no_argument, NULL, 'b'},
        {"field", required_argument, NULL, 'f'},
        {NULL, 0, NULL, 0}};
    /** getopt_long stores the option index here. */
    int option_index = 0;

    while ((c = getopt_long(argc, argv, "jbhvf:", long_options,
                            &option_index)) != -1) {
        switch (c) {
            case 'j':
                options.printJson = 1;
                break;
            case 'b':
                options.onlyBasicInfo = 1;
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
        options.infile = argv[optind++];
    } else {
        usage(0);
    }

    return &options;
}

inline int strendswith(const char *input, const char *tail) {
    const size_t input_strlen = strlen(input);
    const size_t tail_strlen = strlen(tail);
    if (tail_strlen > input_strlen) {
        return 0;
    }

    return strcmp(input + input_strlen - tail_strlen, tail) == 0;
}

typedef struct {
    void *file;
    size_t size;
} infile_struct;

/**
 * @brief Get the contents of the 000 file and return the pointer to it
 *
 * @param file_path
 * @return void*
 */
uint8_t *read_000_file_contents(const char *file_path,
                                      infile_struct *file_info) {
    struct stat s;
    int status;
    size_t size;
    const char *mapped;
    int i;

    /* try to open file */
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open %s failed: %s", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (strendswith(file_path, ".000")) {
        /* Get the size of the file. */
        status = fstat(fd, &s);
        if (status < 0) {
            fprintf(stderr, "stat %s failed: %s", file_path, strerror(errno));
            exit(EXIT_FAILURE);
        }

        file_info->size = s.st_size;

        /* Memory-map the file. */
        mapped = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mapped == MAP_FAILED) {
            fprintf(stderr, "mmap %s failed: %s", file_path, strerror(errno));
        }

        file_info->file = mapped;
        return 1;
    } else {
        // TODO
    }
}

#define CHUNK_SIZE 1024

uint8_t *read_000_file_stream(FILE *stream, infile_struct *file_info) {
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

int main(int argc, char **argv) {
    /** commandline options */
    struct opts *options = get_opts(argc, argv);
    void *file;
    size_t file_size;
    infile_struct *file_info;

    // check if cabextract is available
    if (system("which cabextract > /dev/null 2>&1")) {
        fprintf(stderr,
                "cabextract not found. Please install this dependency. "
                "https://www.cabextract.org.uk/");
        exit(EXIT_FAILURE);
    }

    infile_struct *file_info = malloc(sizeof(infile_struct));

    if (isatty(STDIN_FILENO)) {
        read_000_file_contents(options->infile, file_info);
    } else {
        char c = fgetc(stdin);
        if (c == "M") {
            // CAB file
        } else if (c == 0x64) {
            // 000 File
            file_info = read_000_file_stream(stdin, file_info);
        }
    }
    file = file_info->file;
    file_size = file_info->size;

    if (!file_size) {
        perror("Input size is 0");
        exit(EXIT_FAILURE);
    }

    // Unmap input file. Will do nothing if input file is not actually
    // memory mapped.
    munmap(file, file_size);
}