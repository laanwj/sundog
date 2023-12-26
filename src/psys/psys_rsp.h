/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_PSYS_RSP
#define H_PSYS_RSP

#include "psys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** RSP (Runtime Support Package) call numbers */
enum {
    PSYS_RSP_RELOCSEG      = 0x04,
    PSYS_RSP_MOVESEG       = 0x0e,
    PSYS_RSP_MOVELEFT      = 0x0f,
    PSYS_RSP_MOVERIGHT     = 0x10,
    PSYS_RSP_UNITREAD      = 0x12,
    PSYS_RSP_UNITWRITE     = 0x13,
    PSYS_RSP_TIME          = 0x14,
    PSYS_RSP_FILLCHAR      = 0x15,
    PSYS_RSP_SCAN          = 0x16,
    PSYS_RSP_IOCHECK       = 0x17,
    PSYS_RSP_GETPOOLBYTES  = 0x18,
    PSYS_RSP_PUTPOOLBYTES  = 0x19,
    PSYS_RSP_FLIPSEGBYTES  = 0x1a,
    PSYS_RSP_QUIET         = 0x1b,
    PSYS_RSP_ENABLE        = 0x1c,
    PSYS_RSP_ATTACH        = 0x1d,
    PSYS_RSP_IORESULT      = 0x1e,
    PSYS_RSP_UNITBUSY      = 0x1f,
    PSYS_RSP_POWEROFTEN    = 0x20,
    PSYS_RSP_UNITWAIT      = 0x21,
    PSYS_RSP_UNITCLEAR     = 0x22,
    PSYS_RSP_UNITSTATUS    = 0x24,
    PSYS_RSP_IDSEARCH      = 0x25,
    PSYS_RSP_TREESEARCH    = 0x26,
    PSYS_RSP_READSEG       = 0x27,
    PSYS_RSP_UNITREAD2     = 0x28,
    PSYS_RSP_UNITWRITE2    = 0x29,
    PSYS_RSP_UNITBUSY2     = 0x2a,
    PSYS_RSP_UNITWAIT2     = 0x2b,
    PSYS_RSP_UNITCLEAR2    = 0x2c,
    PSYS_RSP_UNITSTATUS2   = 0x2d,
    PSYS_RSP_READSEG2      = 0x2e,
    PSYS_RSP_SETRESTRICTED = 0x2f,
    /* total count */
    PSYS_RSP_COUNT = 0x30,
};

/** Default unit (device) numbers */
enum {
    PSYS_UNIT_SYSTEM  = 0,
    PSYS_UNIT_CONSOLE = 1,
    PSYS_UNIT_SYSTERM = 2,
    /* 3 is reserved */
    PSYS_UNIT_DISK0   = 4,
    PSYS_UNIT_DISK1   = 5,
    PSYS_UNIT_PRINTER = 6,
    PSYS_UNIT_REMIN   = 7,
    PSYS_UNIT_REMOUT  = 8,
    PSYS_UNIT_DISK2   = 9,
    PSYS_UNIT_DISK3   = 10,
    PSYS_UNIT_DISK4   = 11,
    PSYS_UNIT_DISK5   = 12,
};

/** Global directory size in bytes */
static const int GDIR_SIZE = 0x800;

/** Global directory offsets */
enum {
    GDIR_NUM_ENTRIES = 0x10,
};

/** Global directory entry offsets */
enum {
    GDIR_ENTRY_BLOCK = 0x00,
    GDIR_ENTRY_NAME  = 0x06,
    GDIR_ENTRY_SIZE  = 0x1a,
};

typedef void(psys_rsp_pre_access_hook)(void *data, int disk, unsigned blk, bool wr);

#define PSYS_MAX_EVENTS 64

struct psys_rsp_state {
    struct psys_state *psys;
    psys_byte *disk0;
    unsigned disk0_size;  /* size in blocks */
    unsigned disk0_track; /* track size in blocks */
    bool disk0_wrap;      /* wrap around to beginning */

    bool events_enabled;
    psys_word events[PSYS_MAX_EVENTS]; /* event semaphores */

    /* simulated inputs */
    unsigned time;

    /* hooks */
    psys_rsp_pre_access_hook *pre_access_hook;
    void *pre_access_hook_data;
};

/* Fixed I/O block size */
#define PSYS_BLOCK_SIZE 512

extern struct psys_binding *psys_new_rsp(struct psys_state *state);
extern void psys_rsp_set_disk(struct psys_binding *b, int n, void *data, size_t size, size_t track, bool wrap);
extern void psys_rsp_set_pre_access_hook(struct psys_binding *b, psys_rsp_pre_access_hook *pre_access_hook, void *data);
extern void psys_destroy_rsp(struct psys_binding *b);

/* Trigger event */
extern void psys_rsp_event(struct psys_binding *b, psys_word event, bool taskswitch);

/* Set time returned by rsp */
extern void psys_rsp_settime(struct psys_binding *b, psys_word time);

#ifdef __cplusplus
}
#endif

#endif
