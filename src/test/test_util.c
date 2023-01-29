/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "test_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *load_file(const char *filename, size_t *size_out)
{
    FILE *f       = fopen(filename, "rb");
    char *program = NULL;
    size_t size;
    if (f == NULL) {
        fprintf(stderr, "Cannot open %s\n", filename);
        goto error;
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    program = malloc(size);
    if (!program) {
        fprintf(stderr, "Unable to allocate memory for program\n");
        goto error;
    }
    if (fread(program, 1, size, f) != (size_t)size) {
        fprintf(stderr, "Unable to read data\n");
        goto error;
    }
    fclose(f);
    *size_out = size;
    return program;
error:
    if (f != NULL)
        fclose(f);
    free(program);
    return NULL;
}

void write_file(const char *filename, void *data, size_t size)
{
    FILE *f = fopen(filename, "wb");
    if (f == NULL) {
        fprintf(stderr, "Cannot open %s for writing\n", filename);
        goto cleanup;
    }
    if (fwrite(data, 1, size, f) != (size_t)size) {
        fprintf(stderr, "Cannot write data to file\n");
        goto cleanup;
    }
cleanup:
    if (f != NULL)
        fclose(f);
}
