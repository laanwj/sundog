/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "test_util.h"

#include "util/compat.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void *load_file(const char *filename, size_t *size_out)
{
    int fd        = open(filename, O_RDONLY | O_BINARY);
    char *program = NULL;
    size_t size;
    struct stat stat;
    if (fd < 0) {
        fprintf(stderr, "Cannot open %s\n", filename);
        goto error;
    }
    if (fstat(fd, &stat) < 0) {
        fprintf(stderr, "Cannot stat %s\n", filename);
        goto error;
    }
    size    = stat.st_size;
    program = malloc(size);
    if (!program) {
        fprintf(stderr, "Unable to allocate memory for program\n");
        goto error;
    }
    if (read(fd, program, size) != (ssize_t)size) {
        fprintf(stderr, "Unable to read data\n");
        goto error;
    }
    close(fd);
    *size_out = size;
    return program;
error:
    if (fd >= 0)
        close(fd);
    free(program);
    return NULL;
}

void write_file(const char *filename, void *data, size_t size)
{
    int fd = open(filename, O_WRONLY | O_CREAT | O_BINARY, 0644);
    if (fd < 0) {
        fprintf(stderr, "Cannot open %s for writing\n", filename);
        goto cleanup;
    }
    if (write(fd, data, size) != (ssize_t)size) {
        fprintf(stderr, "Cannot write data to file\n");
        goto cleanup;
    }
cleanup:
    if (fd >= 0)
        close(fd);
}
