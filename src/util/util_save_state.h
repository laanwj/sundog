/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_UTIL_SAVE_STATE
#define H_UTIL_SAVE_STATE

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FD_WRITE(fd, x) (fwrite(&(x), 1, sizeof(x), (fd)) < (size_t)sizeof(x))
#define FD_READ(fd, x) (fread(&(x), 1, sizeof(x), (fd)) < (size_t)sizeof(x))

#ifdef __cplusplus
}
#endif

#endif
