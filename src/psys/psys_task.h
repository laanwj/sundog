/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_PSYS_TASK
#define H_PSYS_TASK

#include "psys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Implementation of SIGNAL op.
 * This signals a semaphore.
 * *taskswitch* determines whether to switch tasks immediately if the signal
 * waked up a higher-priority task. This should usually be true.
 */
extern void psys_signal(struct psys_state *s, psys_word semaphore, bool taskswitch);

/** Implementation of WAIT op.
 * This waits on a semaphore.
 */
extern void psys_wait(struct psys_state *s, psys_word semaphore);

/** Restore VM state from Task Information Block */
extern void psys_restore_state_from_tib(struct psys_state *s);

/** Store VM state to Task Information Block */
extern void psys_store_state_to_tib(struct psys_state *s);

/** Switch to task at head of ready queue.
 * No-op if that task is already running.
 */
extern void psys_task_switch(struct psys_state *s);

#ifdef __cplusplus
}
#endif

#endif
