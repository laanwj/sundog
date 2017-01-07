/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_PSYS_STATE
#define H_PSYS_STATE

#include "psys_types.h"

struct psys_binding {
    struct psys_segment_id seg;
    int num_handlers; /* number of procedures in handlers table */
    /* handlers, starting from 1. They should be zero to not
     * override a particular procedure.
     */
    psys_bindingfunc **handlers;
    /* Save binding state to fd */
    int (*save_state)(struct psys_binding *b, int fd);
    /* Load binding state to fd */
    int (*load_state)(struct psys_binding *b, int fd);

    void *userdata;
};

struct psys_state {
    bool running;
    /* p-system state - program counter can be in high addresses (code
     * pools), all the other pointers must be in the first 64k that can be
     * addressed directly.
     */
    psys_fulladdr ipc;    /* program counter */
    psys_word sp;         /* stack pointer */
    psys_word base;       /* globals base */
    psys_word mp;         /* locals base */
    psys_fulladdr curseg; /* current code segment base */
    psys_word readyq;     /* first task ready to run */
    psys_word curtask;    /* current TIB */
    psys_word erec;       /* current environment record */
    psys_byte curproc;    /* Current procedure number */
    /* PME<->OS state. */
    psys_fulladdr syscom; /* SYSCOM offset (usually 0) */
    /* stored ipc and sp to make instructions restartable, for error handling
     */
    psys_fulladdr stored_ipc; /* stored program counter */
    psys_word stored_sp;      /* stored stack pointer */
    /* all memory addressable to the P-machine, directly (through loads/stores)
     * or indirectly (through readseg/getpoolbytes/putpoolbytes/flipsegbytes)
     */
    psys_byte *memory;
    size_t mem_size;             /* base mem + code pool */
    psys_fulladdr mem_fake_base; /* fake base address for pool base */
    /* External bindings.
     * Binding 0 must always be RSP (Runtime Support Package). This is an array
     * of native functions to handle global intersegment calls on segment 1.
     * Non-zero function pointers will take precedence to p-code or bindings.
     */
    struct psys_binding **bindings;
    unsigned num_bindings;
    /* debug flags */
    unsigned debug;
    /* debugging: function called on every instruction.
     * Put debug hooks and tracing here.
     */
    psys_tracefunc *trace;
    void *trace_userdata;
    /* Hack to initialize stack junk in locals from trace, to prevent
     * it from messing with comparison. If non-zero, sp has changed
     * and new locals have "come into view".
     */
    psys_word local_init_base;
    psys_word local_init_count;
};

#endif
