/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_PSYS_BOOTSTRAP
#define H_PSYS_BOOTSTRAP

#include "psys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct psys_bootstrap_info {
    psys_word boot_unit_id;      /* Boot disk unit number */
    psys_word isp;               /* Initial stack pointer */
    psys_fulladdr mem_fake_base; /* Fake 32-bit memory base address */
    psys_word real_size;         /* Size of REAL in words (0 if no support) */
    /** Not sure what the OS uses these for. This is put in a weird
     * never-documented structure at 0x60, and the code to set it up in PME
     * seems broken (size is always 0?).
     */
    psys_fulladdr ext_mem_base; /* Start of extended memory */
    psys_word ext_mem_size;     /* Size of extended memory in blocks */
};

/**
 * Main method to bootstrap the p-system.
 * Memory must already have been allocated, the rest of the registers are initialized as necessary.
 */
extern void psys_bootstrap(struct psys_state *s, const struct psys_bootstrap_info *boot, const psys_byte *disk);

#ifdef __cplusplus
}
#endif

#endif
