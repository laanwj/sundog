/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "psys_save_state.h"

#include "psys_debug.h"
#include "psys_helpers.h"
#include "psys_state.h"
#include "util/util_save_state.h"

#include <string.h>

/* Header for savestates */
#define PSYS_STATE_ID 0x50535953

int psys_save_state(struct psys_state *s, FILE *fd)
{
    int rv;
    unsigned x;
    uint32_t id = PSYS_STATE_ID;
    /* Save VM state */
    if (FD_WRITE(fd, id)
        || FD_WRITE(fd, s->ipc)
        || FD_WRITE(fd, s->sp)
        || FD_WRITE(fd, s->base)
        || FD_WRITE(fd, s->mp)
        || FD_WRITE(fd, s->curseg)
        || FD_WRITE(fd, s->readyq)
        || FD_WRITE(fd, s->curtask)
        || FD_WRITE(fd, s->erec)
        || FD_WRITE(fd, s->curproc)
        || FD_WRITE(fd, s->syscom)
        || FD_WRITE(fd, s->stored_ipc)
        || FD_WRITE(fd, s->stored_sp)
        || FD_WRITE(fd, s->mem_size)
        || FD_WRITE(fd, s->mem_fake_base)) {
        return -1;
    }
    if (fwrite(s->memory, 1, s->mem_size, fd) < (size_t)s->mem_size) {
        return -1;
    }
    /* Save state of bindings */
    for (x = 0; x < s->num_bindings; ++x) {
        if (s->bindings[x]->save_state) {
            rv = s->bindings[x]->save_state(s->bindings[x], fd);
            if (rv < 0) {
                return rv;
            }
        }
    }
    return 0;
}

int psys_load_state(struct psys_state *s, FILE *fd)
{
    int rv;
    unsigned x;
    uint32_t id;
    /* Load VM state */
    if (FD_READ(fd, id)) {
        return -1;
    }
    if (id != PSYS_STATE_ID) {
        psys_debug("Invalid psys state record %08x\n", id);
        return -1;
    }

    if (FD_READ(fd, s->ipc)
        || FD_READ(fd, s->sp)
        || FD_READ(fd, s->base)
        || FD_READ(fd, s->mp)
        || FD_READ(fd, s->curseg)
        || FD_READ(fd, s->readyq)
        || FD_READ(fd, s->curtask)
        || FD_READ(fd, s->erec)
        || FD_READ(fd, s->curproc)
        || FD_READ(fd, s->syscom)
        || FD_READ(fd, s->stored_ipc)
        || FD_READ(fd, s->stored_sp)
        || FD_READ(fd, s->mem_size)
        || FD_READ(fd, s->mem_fake_base)) {
        return -1;
    }
    if (fread(s->memory, 1, s->mem_size, fd) < (size_t)s->mem_size) {
        return -1;
    }
    /* Load state of bindings */
    for (x = 0; x < s->num_bindings; ++x) {
        if (s->bindings[x]->load_state) {
            rv = s->bindings[x]->load_state(s->bindings[x], fd);
            if (rv < 0) {
                return rv;
            }
        }
    }
    return 0;
}
