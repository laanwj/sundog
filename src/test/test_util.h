/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_TEST_UTIL
#define H_TEST_UTIL

#include <stdlib.h>

void *load_file(const char *filename, size_t *size_out);
void write_file(const char *filename, void *data, size_t size);

#endif
