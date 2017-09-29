/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_PSYS_SAVE_STATE
#define H_PSYS_SAVE_STATE

#include "psys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Save state to fd (return 0 on success) */
extern int psys_save_state(struct psys_state *b, int fd);

/** Load state from fd (return 0 on success) */
extern int psys_load_state(struct psys_state *b, int fd);

#ifdef __cplusplus
}
#endif

#endif
