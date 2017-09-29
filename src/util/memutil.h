/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_MEMUTIL
#define H_MEMUTIL

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define CALLOC_STRUCT(T) (struct T *)calloc(1, sizeof(struct T))

#ifdef __cplusplus
}
#endif

#endif
