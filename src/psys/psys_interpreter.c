/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "psys_interpreter.h"

#include "psys_constants.h"
#include "psys_debug.h"
#include "psys_helpers.h"
#include "psys_opcodes.h"
#include "psys_registers.h"
#include "psys_set.h"
#include "psys_task.h"
#include "util/util_minmax.h"

#include <string.h>

/** Helpers for interpreter ***/

/* Boolean test. In contrast to how C handles booleans, p-system booleans only
 * use the bottom bit. This means that 0 is false, but e.g. 2, 4, 6 and so on
 * is also false.
 */
#define BOOL(x) ((x)&1)

/** Instruction fetching ***/

/* Read unsigned byte from PC (UB/DB) */
static inline psys_byte fetch_UB(struct psys_state *state)
{
    return psys_ldb(state, 0, state->ipc++);
}

/* Read signed byte from PC (SB) */
static inline int fetch_SB(struct psys_state *state)
{
    return signext_byte(psys_ldb(state, 0, state->ipc++));
}

/* Read signed LE word from PC (W) */
static inline int fetch_W(struct psys_state *state)
{
    psys_word a = psys_ldb(state, 0, state->ipc++);
    psys_word b = psys_ldb(state, 0, state->ipc++);
    return signext_word(a | (b << 8));
}

/* Read BIG (1/2 byte varlen) from PC (B).
 * I've called this V instead of B because otherwise it's too confusing
 * with UB/DB/SB arguments. */
static inline psys_word fetch_V(struct psys_state *state)
{
    psys_word a = psys_ldb(state, 0, state->ipc++);
    if (a >= 0x80) {
        a = ((a & 0x7f) << 8) | psys_ldb(state, 0, state->ipc++);
    }
    return a;
}

/** Look up intermediate MSCW offset, lexlevel levels
 * above the current one (0=local). This is used for intermediate (e.g. nested
 * scope) variable accesses.
 */
static inline psys_fulladdr intermd_mscw(struct psys_state *state, unsigned lexlevel)
{
    unsigned l;
    psys_fulladdr mscw = state->mp;
    for (l = 0; l < lexlevel; ++l) {
        mscw = psys_ldw(state, mscw + PSYS_MSCW_MSSTAT);
    }
    return mscw;
}

/** Get base address of a pool, given the address of a pool descriptor structure.
 */
psys_fulladdr psys_pool_get_base(struct psys_state *s, psys_fulladdr pool)
{
    psys_fulladdr poolbase = 0; /* internal pool */
    if (pool != PSYS_NIL) {     /* external pool */
        poolbase =              /* combine low and high word (XXX does this depend on endian?) */
            (((psys_ldw(s, W(pool + PSYS_PD_PoolBase, 0))) << 16) | psys_ldw(s, W(pool + PSYS_PD_PoolBase, 1)))
            - s->mem_fake_base;
    }
    return poolbase;
}

/* Look up the segment base address and add offset, given an erec offset.
 * Fault if a segment is not resident, and return PSYS_ADDR_ERROR.
 */
psys_fulladdr psys_segment_from_erec(struct psys_state *s, psys_fulladdr erec)
{
    psys_fulladdr sib     = psys_ldw(s, erec + PSYS_EREC_Env_SIB);
    psys_fulladdr poolofs = psys_ldw(s, sib + PSYS_SIB_Seg_Base);
    if (PDBG(s, CALL)) {
        psys_debug("lookup[%.8s]", psys_bytes(s, sib + PSYS_SIB_Seg_Name));
    }
    if (poolofs == PSYS_NIL) { /* segment not resident - fault */
        psys_fault(s, s->curtask, erec, 0, PSYS_FAULT_SEG);
        return PSYS_ADDR_ERROR;
    } else { /* segment resident */
        psys_fulladdr pool     = psys_ldw(s, sib + PSYS_SIB_Seg_Pool);
        psys_fulladdr poolbase = psys_pool_get_base(s, pool);
        if (PDBG(s, CALL)) {
            psys_debug("  %08x <- pool %08x: %08x + %04x\n", poolbase + poolofs, pool, poolbase, poolofs);
        }
        return poolbase + poolofs;
    }
}

/* Look up EREC for segment number in local segment environment (EVEC).
 * Returns 0 if segment not found.
 */
static inline psys_fulladdr lookup_ref_segment(struct psys_state *s, psys_word segid)
{
    /* Atari ST p-system interpreter caches the current evec. I don't think
     * it makes sense for us to do so as it's just one extra lookup and memory
     * is fast enough these days...
     */
    psys_fulladdr evec = psys_ldw(s, s->erec + PSYS_EREC_Env_Vect);
    psys_word length   = psys_ldw(s, evec + PSYS_EVEC_Vec_Length);
    if (segid > length) { /* out of range */
        if (PDBG(s, WARNING)) {
            psys_debug("segment id out of range");
        }
        psys_execerror(s, PSYS_ERR_NOPROC);
        return PSYS_ADDR_ERROR;
    } else {
        psys_word addr = psys_ldw(s, W(evec + PSYS_EVEC_Vec_Length, segid));
        if (addr == 0) { /* no entry */
            if (PDBG(s, WARNING)) {
                psys_debug("no entry for segment %d in evec\n", segid);
            }
            psys_execerror(s, PSYS_ERR_NOPROC);
            return PSYS_ADDR_ERROR;
        }
        return addr;
    }
}

/** Get address of local variable */
static inline psys_fulladdr local_addr(struct psys_state *state, unsigned n)
{
    return W(state->mp + PSYS_MSCW_VAROFS, n);
}

/** Get address of global variable */
static inline psys_fulladdr global_addr(struct psys_state *state, unsigned n)
{
    return W(state->base + PSYS_MSCW_VAROFS, n);
}

/** Get address of intermediate variable */
static inline psys_fulladdr intermd_addr(struct psys_state *state, unsigned lexlevel, unsigned n)
{
    return W(intermd_mscw(state, lexlevel) + PSYS_MSCW_VAROFS, n);
}

/** Get address of extended variable (a global address from another segment).
 * This can cause a PSYS_ERR_NOPROC error if the segment could not be found.
 * It does not matter whether the segment is resident: globals are not swapped.
 */
static inline psys_fulladdr extended_addr(struct psys_state *state, unsigned seg, unsigned n)
{
    psys_fulladdr ext_erec = lookup_ref_segment(state, seg);
    if (ext_erec != PSYS_ADDR_ERROR) { /* continue only if segment could be found */
        psys_word ext_base = psys_ldw(state, ext_erec + PSYS_EREC_Env_Data);
        return W(ext_base + PSYS_MSCW_VAROFS, n);
    } else {
        return PSYS_ADDR_ERROR;
    }
}

/** Look up address for string descriptor.
 * A string descriptor is a (erec,ofs) tuple in memory. If erec is NIL,
 * the offset is an offset in memory, if erec is non-null it is an offset
 * into the segment pointed to by the environment record.
 * Fault if a segment is not resident, and return PSYS_ADDR_ERROR.
 */
static psys_fulladdr array_descriptor_to_addr(struct psys_state *s, psys_fulladdr straddr)
{
    psys_fulladdr erec = psys_ldw(s, W(straddr, 0));
    psys_fulladdr ofs  = psys_ldw(s, W(straddr, 1));
    if (erec == PSYS_NIL) { /* memory */
        return ofs;
    } else { /* segment */
        psys_fulladdr segment = psys_segment_from_erec(s, erec);
        if (segment == PSYS_ADDR_ERROR) { /* segment not resident */
            return PSYS_ADDR_ERROR;
        } else {
            return segment + ofs;
        }
    }
    return 0;
}

/** Compute constant pool offset + n words, given a segment base address */
static inline psys_fulladdr seg_cpool_ofs(struct psys_state *state, psys_fulladdr seg, unsigned n)
{
    return W(0, psys_ldw(state, seg + PSYS_SEG_CPOOLOFS) + n);
}

/** Is segment in our local endian? */
static inline bool seg_needs_endian_flip(struct psys_state *state, psys_fulladdr seg)
{
    return psys_ldw(state, seg + PSYS_SEG_ENDIAN) != PSYS_ENDIAN_NATIVE;
}

/** Increase timestamp in SYSCOM */
void psys_increase_timestamp(struct psys_state *state, psys_fulladdr erec)
{
    psys_word timestamp = psys_ldw(state, state->syscom + PSYS_SYSCOM_TIMESTAMP);
    psys_word sib       = psys_ldw(state, erec + PSYS_EREC_Env_SIB);
    timestamp++;
    psys_stw(state, state->syscom + PSYS_SYSCOM_TIMESTAMP, timestamp);
    psys_stw(state, sib + PSYS_SIB_Time_Stamp, timestamp);
}

/** Increase or decrease reference count on segment.
 * Increase Seg_Refs count in SIB to prevent the segment from being swapped out.
 * Decrease Seg_Refs count in SIB to allow the segment to be swapped out again.
 */
static void psys_segment_refcount(struct psys_state *state, psys_fulladdr erec, int delta)
{
    psys_word sib = psys_ldw(state, erec + PSYS_EREC_Env_SIB);
    psys_stw(state, sib + PSYS_SIB_Link_Count,
        psys_ldw(state, sib + PSYS_SIB_Link_Count) + delta);
}

/** Look up procedure address from erec and procedure number. */
static psys_fulladdr lookup_procedure(struct psys_state *s, psys_fulladdr erec, psys_word procedure, psys_word *num_locals_out, psys_word *end_pointer, psys_fulladdr *segment_out)
{
    psys_fulladdr sib = psys_ldw(s, erec + PSYS_EREC_Env_SIB);
    psys_word num_locals;
    if (PDBG(s, CALL)) {
        psys_debug("-> %.8s:0x%02x sib=0x%04x\n", psys_bytes(s, sib + PSYS_SIB_Seg_Name), procedure, sib);
    }
    psys_fulladdr segment;
    if (erec == s->erec) { /* current erec - take a shortcut */
        segment = s->curseg;
    } else { /* cross-segment */
        segment = psys_segment_from_erec(s, erec);
        if (segment == PSYS_ADDR_ERROR) { /* not resident, bail out */
            return PSYS_ADDR_ERROR;
        }
    }
    /* The procedure dictionary is flipped to native endian on load, so no need
     * to flip these. Same for the number of locals. */
    psys_word procdict = psys_ldw(s, segment + PSYS_SEG_PROCDICT);
    psys_word numproc  = psys_ldw(s, W(segment, procdict));
    if (PDBG(s, CALL)) {
        psys_debug("  segment=0x%05x procdict=0x%05x numproc=%04x\n", segment, procdict, numproc);
    }
    if (procedure == 0 || procedure > numproc) {
        /* procedure number out of range */
        psys_execerror(s, PSYS_ERR_NOPROC);
        return PSYS_ADDR_ERROR;
    }
    psys_fulladdr procaddr = W(segment, psys_ldw(s, W(segment, procdict - procedure)));
    num_locals             = psys_ldw(s, procaddr);
    if (num_locals_out) {
        *num_locals_out = num_locals;
    }
    if (end_pointer) {
        *end_pointer = psys_ldw(s, W(procaddr, -1));
    }
    if (segment_out) {
        *segment_out = segment;
    }
    if (PDBG(s, CALL)) {
        psys_debug("  procaddr=0x%05x num_locals=%04x\n", procaddr, num_locals);
    }
    return procaddr;
}

/** Call a native procedure provided by a binding.
 * Return true if the procedure was executed and false on error.
 */
static bool call_binding(struct psys_state *s, psys_fulladdr segment, psys_word procedure, psys_fulladdr env_data)
{
    struct psys_segment_id id;
    unsigned x;
    const struct psys_binding *found = NULL;
    memcpy(&id, psys_bytes(s, segment + PSYS_SEG_NAME), 8);
    for (x = 0; x < s->num_bindings; ++x) { /* Linear search for now, should be fast enough */
        if (s->bindings[x]->seg.num == id.num) {
            found = s->bindings[x];
        }
    }
    if (!found || procedure >= found->num_handlers || !found->handlers[procedure])
        return false;
    found->handlers[procedure](s, found->userdata, segment, env_data);
    return true;
}

/** Procedure call after context lookups (except of the procedure address itself).
 * msstat can be 0, in which case the current base pointer will be used.
 */
static void handle_call_formal(struct psys_state *s, psys_fulladdr msstat, psys_fulladdr erec, psys_word procedure, bool nonlocal)
{
    psys_fulladdr funcaddr, newseg;
    psys_word num_locals;
    if (PDBG(s, CALL)) {
        psys_debug("call msstat=0x%04x erec=0x%04x proc=0x%02x\n", msstat, erec, procedure);
    }
    if (!msstat) {
        msstat = s->base;
    }
    funcaddr = lookup_procedure(s, erec, procedure, &num_locals, NULL, &newseg);
    if (funcaddr == PSYS_ADDR_ERROR) { /* function out of range or not resident */
        return;
    }
    if (num_locals == 0xffff) { /* native */
        if (!call_binding(s, newseg, procedure, psys_ldw(s, erec + PSYS_EREC_Env_Data))) {
            psys_panic("NATIVE call to %.8s:0x%02x not handled\n",
                psys_bytes(s, newseg + PSYS_SEG_NAME), procedure);
        }
        return;
    }
    if (nonlocal) { /* increase timestamp for intersegment call */
        psys_increase_timestamp(s, s->erec);
        psys_segment_refcount(s, s->erec, 1);
    }
    /* Push locals */
    s->local_init_base  = 5;
    s->local_init_count = num_locals;
    s->sp               = W(s->sp, -num_locals);
    /* Push MSCW */
    psys_push(s, s->curproc);         /* MPROC */
    psys_push(s, s->erec);            /* MSENV */
    psys_push(s, s->ipc - s->curseg); /* IPC */
    psys_push(s, s->mp);              /* MSDYN */
    psys_push(s, msstat);             /* MSSTAT */
    /* Set up registers for new procedure */
    s->mp      = s->sp;
    s->base    = psys_ldw(s, erec + PSYS_EREC_Env_Data);
    s->curseg  = newseg;
    s->ipc     = W(funcaddr, 1);
    s->erec    = erec;
    s->curproc = procedure;
    if (PDBG(s, CALL)) {
        psys_debug("after call: mp=0x%04x ipc=0x%05x erec=0x%04x curproc=0x%02x\n",
            s->mp, s->ipc, s->erec, s->curproc);
    }
}

/* handle_call: fake segment id for "this segment" */
static const int CALL_CURSEG = 0xffff;
/* handle_call: fake lex level for global */
static const int CALL_GLOBAL = 0xffff;
/* handle_call: lex level for local */
static const int CALL_LOCAL = 0;

/* Handle any kind of procedure call instruction except CFP.
 * This can fail for various reasons:
 *   - segment not found in segment table (PSYS_ERR_NOPROC)
 *   - segment not resident (PSYS_FAULT_SEG)
 */
static void handle_call(struct psys_state *s, psys_word seg, psys_word lexlevel, psys_word procedure)
{
    /* Determine target erec */
    psys_fulladdr erec;
    psys_fulladdr msstat = 0;
    bool nonlocal        = false;
    if (seg == 1 && lexlevel == CALL_GLOBAL) { /* RSP */
        struct psys_binding *rsp = (s->num_bindings > 0) ? s->bindings[0] : NULL;
        if (rsp != NULL && procedure < rsp->num_handlers && rsp->handlers[procedure]) {
            rsp->handlers[procedure](s, rsp->userdata, 0, 0);
            return;
        }
        /* just fall through if not found so that KERNEL can handle it */
    }
    if (seg != CALL_CURSEG) { /* lookup segment */
        erec     = lookup_ref_segment(s, seg);
        nonlocal = true;
    } else { /* current segment */
        erec = s->erec;
    }
    if (erec == PSYS_ADDR_ERROR) /* lookup error - don't continue */
        return;
    /* Determine static link (closure) */
    if (lexlevel != CALL_GLOBAL) {
        msstat = intermd_mscw(s, lexlevel);
    }
    handle_call_formal(s, msstat, erec, procedure, nonlocal);
}

/* Handle RPU (return) instruction. This can generate a segment fault if the
 * segment returned to is not resident.
 */
static void handle_return(struct psys_state *s, psys_word count)
{
    /* Store original mp as registers will be changed */
    psys_fulladdr mp          = s->mp;
    psys_fulladdr caller_erec = psys_ldw(s, mp + PSYS_MSCW_MSENV);
    psys_fulladdr caller_seg;
    psys_sword caller_proc;

    /* Bail out early if segment not resident, to make it possible
     * to re-execute the instruction. */
    caller_seg = psys_segment_from_erec(s, caller_erec);
    if (caller_seg == PSYS_ADDR_ERROR) {
        return;
    }

    if (caller_erec != s->erec) { /* increase timestamp for intersegment return */
        psys_increase_timestamp(s, s->erec);
        psys_segment_refcount(s, s->erec, -1);
    }

    /* Restore register state from MSCW */
    s->mp       = psys_ldw(s, mp + PSYS_MSCW_MSDYN);
    s->base     = psys_ldw(s, caller_erec + PSYS_EREC_Env_Data);
    s->erec     = caller_erec;
    s->curseg   = caller_seg;
    caller_proc = psys_ldsw(s, mp + PSYS_MSCW_MPROC);
    if (caller_proc >= 0) {
        s->curproc = caller_proc;
        s->ipc     = caller_seg + psys_ldw(s, mp + PSYS_MSCW_IPC);
        if (PDBG(s, CALL)) {
            psys_debug("return to %05x (erec %05x procedure %02x)\n", s->ipc, s->erec, s->curproc);
        }
    } else { /* Returning to a negative procedure number means we need to jump to the end pointer of that procedure */
        psys_word end_addr;
        if (lookup_procedure(s, caller_erec, -caller_proc, NULL, &end_addr, NULL) == PSYS_ADDR_ERROR) {
            psys_panic("Segment not resident during error return\n");
        }
        s->curproc = -caller_proc;
        s->ipc     = caller_seg + end_addr;
        if (PDBG(s, CALL)) {
            psys_debug("return (to end) to %05x (erec %05x procedure %02x)\n", s->ipc, s->erec, s->curproc);
        }
    }

    /* Restore SP, add 'count' words to remove locals + parameters */
    s->sp = W(mp + PSYS_MSCW_SIZE, count);

    if (PDBG(s, CALL)) {
        psys_debug("after return: mp=0x%04x ipc=0x%05x erec=0x%04x curproc=0x%02x\n",
            s->mp, s->ipc, s->erec, s->curproc);
    }
}

/* Return address of data.
 * If flag is 0, ofs points to memory.
 * If flag is 1, ofs points to constant pool of current segment.
 * Current segment will always be resident, so this cannot cause a segment fault.
 */
static psys_fulladdr memory_or_segment_addr(struct psys_state *s, psys_word flag, psys_word ofs)
{
    if (flag == 0) { /* memory */
        return ofs;
    } else if (flag == 1 || flag == 2) { /* segment */
        return s->curseg + ofs;
    } else {
        psys_panic("Unknown memory or segment address flag %i", flag);
        return 0; /* never reached */
    }
}

/* Byte array comparison
 * return 0 if arrays match
 *        <0 if s1<s2
 *        >0 if s1>s2
 */
static int compare_bytearrays(struct psys_state *s, psys_word flag1, psys_word ofs1, psys_word flag2, psys_word ofs2, psys_word size)
{
    psys_fulladdr addr1 = memory_or_segment_addr(s, flag1, ofs1);
    psys_fulladdr addr2 = memory_or_segment_addr(s, flag2, ofs2);
    return memcmp(psys_bytes(s, addr1), psys_bytes(s, addr2), size);
}

/* Strings comparison.
 * return 0 if strings match
 *        <0 if s1<s2
 *        >0 if s1>s2
 */
static int compare_strings(struct psys_state *s, psys_word flag1, psys_word ofs1, psys_word flag2, psys_word ofs2)
{
    const psys_byte *s1 = psys_bytes(s, memory_or_segment_addr(s, flag1, ofs1));
    const psys_byte *s2 = psys_bytes(s, memory_or_segment_addr(s, flag2, ofs2));
    /* determine number of bytes to compare: this will be the minimum length of both strings */
    int minlen = umin(s1[0], s2[0]);
    /* do straight memory comparison, if the string differ return the result */
    int cmp = memcmp(s1 + 1, s2 + 1, minlen);
    if (cmp != 0)
        return cmp;
    /* if the first parts of the strings match, return comparison result for lengths */
    return s1[0] - s2[0];
}

void psys_interpreter(struct psys_state *s)
{
    /* check:
     * - word/sword usage for ops
     * - do stack checks for ADJ etc
     * - Importent: properly generate segment faults for the following instructions:
     *   CAP, CSP, CXL, SCXGn, CXG, CXI, CFP, RPU, SIGNAL (if a task switch occurs), WAIT (if a task switch occurs)
     *   These have bene marked *segf*
     * - Less important: generate stack fault for:
     *   LDC, LDM, ADJ, SRS, CLP, CGP, SCIPn, CIP, CXL, SCXGn, CXG, CXI, CFP
     *   This does not seem to be implemented in the Atari ST version.
     */
    // clang-format off
    static const void *dispatch_table[256] = {
    &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,
    &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,
    &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,
    &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,      &&do_sldc,
    &&do_sldl,      &&do_sldl,      &&do_sldl,      &&do_sldl,      &&do_sldl,      &&do_sldl,      &&do_sldl,      &&do_sldl,
    &&do_sldl,      &&do_sldl,      &&do_sldl,      &&do_sldl,      &&do_sldl,      &&do_sldl,      &&do_sldl,      &&do_sldl,
    &&do_sldo,      &&do_sldo,      &&do_sldo,      &&do_sldo,      &&do_sldo,      &&do_sldo,      &&do_sldo,      &&do_sldo,
    &&do_sldo,      &&do_sldo,      &&do_sldo,      &&do_sldo,      &&do_sldo,      &&do_sldo,      &&do_sldo,      &&do_sldo,
    &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,
    &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,
    &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,
    &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,
    &&do_slla,      &&do_slla,      &&do_slla,      &&do_slla,      &&do_slla,      &&do_slla,      &&do_slla,      &&do_slla,
    &&do_sstl,      &&do_sstl,      &&do_sstl,      &&do_sstl,      &&do_sstl,      &&do_sstl,      &&do_sstl,      &&do_sstl,
    &&do_scxg,      &&do_scxg,      &&do_scxg,      &&do_scxg,      &&do_scxg,      &&do_scxg,      &&do_scxg,      &&do_scxg,
    &&do_sind,      &&do_sind,      &&do_sind,      &&do_sind,      &&do_sind,      &&do_sind,      &&do_sind,      &&do_sind,
    &&do_ldcb,      &&do_ldci,      &&do_lco,       &&do_ldc,       &&do_lla,       &&do_ldo,       &&do_lao,       &&do_ldl,
    &&do_lda,       &&do_lod,       &&do_ujp,       &&do_ujpl,      &&do_mpi,       &&do_dvi,       &&do_stm,       &&do_modi,
    &&do_clp,       &&do_cgp,       &&do_cip,       &&do_cxl,       &&do_cxg,       &&do_cxi,       &&do_rpu,       &&do_cfp,
    &&do_ldcn,      &&do_lsl,       &&do_lde,       &&do_lae,       &&do_nop,       &&do_lpr,       &&do_bpt,       &&do_bnot,
    &&do_lor,       &&do_land,      &&do_adi,       &&do_sbi,       &&do_stl,       &&do_sro,       &&do_str,       &&do_ldb,
    &&do_native,    &&do_nat_info,  &&do_invalid,   &&do_cap,       &&do_csp,       &&do_slod,      &&do_slod,      &&do_invalid,
    &&do_equi,      &&do_neqi,      &&do_leqi,      &&do_geqi,      &&do_leusw,     &&do_geusw,     &&do_eqpwr,     &&do_lepwr,
    &&do_gepwr,     &&do_eqbyte,    &&do_lebyte,    &&do_gebyte,    &&do_srs,       &&do_swap,      &&do_invalid,   &&do_invalid,
    &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_sto,       &&do_mov,       &&do_dup2,      &&do_adj,
    &&do_stb,       &&do_ldp,       &&do_stp,       &&do_chk,       &&do_flt,       &&do_eqreal,    &&do_lereal,    &&do_gereal,
    &&do_ldm,       &&do_spr,       &&do_efj,       &&do_nfj,       &&do_fjp,       &&do_fjpl,      &&do_xjp,       &&do_ixa,
    &&do_ixp,       &&do_ste,       &&do_inn,       &&do_uni,       &&do_int,       &&do_dif,       &&do_signal,    &&do_wait,
    &&do_abi,       &&do_ngi,       &&do_dup1,      &&do_abr,       &&do_ngr,       &&do_lnot,      &&do_ind,       &&do_inc,
    &&do_eqstr,     &&do_lestr,     &&do_gestr,     &&do_astr,      &&do_cstr,      &&do_inci,      &&do_deci,      &&do_scip,
    &&do_scip,      &&do_tjp,       &&do_ldcrl,     &&do_ldrl,      &&do_strl,      &&do_invalid,   &&do_invalid,   &&do_invalid,
    &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid,   &&do_invalid
    };
    int op; /* Stored opcode */
    int arg0,arg1,arg2; /* Instruction arguments */
    int tos0,tos1,tos2,tos3; /* Top Of Stack,-1, -2, -3, ... */
    int x; /* Loop variable */
    s->running = true;
#define STORE_STATE() s->stored_sp = s->sp; s->stored_ipc = s->ipc
#define DISPATCH() if (!s->running) return; if (s->trace) s->trace(s, s->trace_userdata); STORE_STATE(); op = fetch_UB(s); goto *dispatch_table[op]
do_nop: /* No operation */
    DISPATCH();
    while(1) {
        do_sldc: /* Short load constant */
            psys_push(s, op - PSOP_SLDC0);
            DISPATCH();
        do_sldl: /* Short load local */
            psys_push(s, psys_ldw(s, local_addr(s, op - PSOP_SLDL1 + 1)));
            DISPATCH();
        do_sldo: /* Short local global */
            psys_push(s, psys_ldw(s, global_addr(s, op - PSOP_SLDO1 + 1)));
            DISPATCH();
        do_slla: /* Short load local address */
            psys_push(s, local_addr(s, op - PSOP_SLLA1 + 1));
            DISPATCH();
        do_sstl: /* Short store local */
            tos0 = psys_pop(s);
            psys_stw(s, local_addr(s, op - PSOP_SSTL1 + 1), tos0);
            DISPATCH();
        do_scxg: /* Short call intersegment */ /* segf */
            arg0 = fetch_UB(s);
            handle_call(s, op - PSOP_SCXG1 + 1, CALL_GLOBAL, arg0);
            DISPATCH();
        do_sind: /* Short index */
            tos0 = psys_pop(s);
#if 0
            psys_debug("SIND %05x\n", W(tos0, op - PSOP_SIND0));
            psys_debug_hexdump(s, tos0, 32);
#endif
            psys_push(s, psys_ldw(s, W(tos0, op - PSOP_SIND0)));
            DISPATCH();
        do_ldcb: /* Load constant (unsigned) byte */
            arg0 = fetch_UB(s);
            psys_push(s, arg0);
            DISPATCH();
        do_ldci:  /* Load constant integer */
            arg0 = fetch_W(s);
            psys_push(s, arg0);
            DISPATCH();
        do_lco: /* Load constant offset */
            arg0 = fetch_V(s);
            psys_push(s, seg_cpool_ofs(s, s->curseg, arg0));
            DISPATCH();
        do_ldc: { /* Load constant (words) */
            psys_fulladdr src;
            arg0 = fetch_UB(s); /* flag: 0 keep as is, 2 flip endian if necessary */
            arg1 = fetch_V(s);  /* word offset into current segment */
            arg2 = fetch_UB(s); /* number of words */
            src = s->curseg + seg_cpool_ofs(s, s->curseg, arg1);

            /* perform endian swap if requested and necessary.
             * push in reversed order because the words should appear in the same order.
             */
            if (arg0 == 2 && seg_needs_endian_flip(s, s->curseg)) {
                for (x=arg2-1; x>=0; --x) {
                    psys_push(s, psys_flip_endian(psys_ldw(s, W(src, x))));
                }
            } else {
                for (x=arg2-1; x>=0; --x) {
                    psys_push(s, psys_ldw(s, W(src, x)));
                }
            }
            } DISPATCH();
        do_lla: /* Load local address */
            arg0 = fetch_V(s);
            psys_push(s, local_addr(s, arg0));
            DISPATCH();
        do_ldo: /* Load global */
            arg0 = fetch_V(s);
            psys_push(s, psys_ldw(s, global_addr(s, arg0)));
            DISPATCH();
        do_lao: /* Load global address */
            arg0 = fetch_V(s);
            psys_push(s, global_addr(s, arg0));
            DISPATCH();
        do_ldl: /* Load local */
            arg0 = fetch_V(s);
            psys_push(s, psys_ldw(s, local_addr(s, arg0)));
            DISPATCH();
        do_lda: /* Load intermediate address */
            arg0 = fetch_UB(s);
            arg1 = fetch_V(s);
            psys_push(s, intermd_addr(s, arg0, arg1));
            DISPATCH();
        do_lod: /* Load intermediate */
            arg0 = fetch_UB(s);
            arg1 = fetch_V(s);
            psys_push(s, psys_ldw(s, intermd_addr(s, arg0, arg1)));
            DISPATCH();
        do_ujp: /* Unconditional jump */
            s->ipc += fetch_SB(s);
            DISPATCH();
        do_ujpl: /* Unconditional jump long */
            s->ipc += fetch_W(s);
            DISPATCH();
        do_mpi: /* Multiply (unsigned) integer */
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, tos1 * tos0);
            DISPATCH();
        do_dvi: /* Divide (signed) integer */
            tos0 = psys_spop(s);
            tos1 = psys_spop(s);
            if (tos0 == 0) {
                psys_execerror(s, PSYS_ERR_DIVZER);
            } else {
                psys_push(s, tos1 / tos0);
            }
            DISPATCH();
        do_stm: { /* Store multiple */
            psys_fulladdr dst;
            arg0 = fetch_UB(s);
            dst = psys_ldw(s, W(s->sp, arg0));
            for (x=0; x<arg0; ++x) {
                psys_stw(s, W(dst, x), psys_pop(s));
            }
            psys_pop(s); /* pop dst */
            } DISPATCH();
        do_modi: /* Modulo integers */
            tos0 = psys_spop(s);
            tos1 = psys_spop(s);
            if (tos0 == 0) {
                psys_execerror(s, PSYS_ERR_DIVZER);
            } else {
                psys_sword r = tos1 % tos0;
                /* p-systems interpretation of MOD always returns positive numbers */
                psys_push(s, (r < 0)?(r+tos0):r);
            }
            DISPATCH();
        do_clp: /* Call local procedure */
            arg0 = fetch_UB(s);
            handle_call(s, CALL_CURSEG, CALL_LOCAL, arg0);
            DISPATCH();
        do_cgp: /* Call global procedure */
            arg0 = fetch_UB(s);
            handle_call(s, CALL_CURSEG, CALL_GLOBAL, arg0);
            DISPATCH();
        do_cip: /* Call intermediate procedure */
            arg0 = fetch_UB(s);
            arg1 = fetch_UB(s);
            handle_call(s, CALL_CURSEG, arg0, arg1);
            DISPATCH();
        do_cxl: /* Call intersegment local procedure */ /* segf */
            arg0 = fetch_UB(s);
            arg1 = fetch_UB(s);
            handle_call(s, arg0, CALL_LOCAL, arg1);
            DISPATCH();
        do_cxg: /* Call intersegment global procedure */ /* segf */
            arg0 = fetch_UB(s);
            arg1 = fetch_UB(s);
            handle_call(s, arg0, CALL_GLOBAL, arg1);
            DISPATCH();
        do_cxi: /* Call intersegment intermediate procedure */ /* segf */
            arg0 = fetch_UB(s);
            arg1 = fetch_UB(s);
            arg2 = fetch_UB(s);
            handle_call(s, arg0, arg1, arg2);
            DISPATCH();
        do_rpu: /* Return from procedure */ /* segf */
            arg0 = fetch_V(s);
            handle_return(s, arg0);
            DISPATCH();
        do_cfp: /* Call formal procedure */ /* segf */
            /* In contrast to what the p-systems internal reference manual says, this has no argument */
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            tos2 = psys_pop(s);
            handle_call_formal(s, tos0, tos1, tos2, true);
            DISPATCH();
        do_ldcn: /* Load constant NIL */
            psys_push(s, PSYS_NIL);
            DISPATCH();
        do_lsl: /* Load static link */
            arg0 = fetch_UB(s);
            psys_push(s, intermd_mscw(s, arg0));
            DISPATCH();
        do_lde: { /* Load extended */
            psys_fulladdr addr;
            arg0 = fetch_UB(s);
            arg1 = fetch_V(s);
            addr = extended_addr(s, arg0, arg1);
            if (addr != PSYS_ADDR_ERROR) { /* continue only if segment could be found - if not, error will already have been set */
                psys_push(s, psys_ldw(s, addr));
            }
            } DISPATCH();
        do_lae: { /* Load address extended */
            psys_fulladdr addr;
            arg0 = fetch_UB(s);
            arg1 = fetch_V(s);
            addr = extended_addr(s, arg0, arg1);
            if (addr != PSYS_ADDR_ERROR) { /* continue only if segment could be found - if not, error will already have been set */
                psys_push(s, addr);
            }
            } DISPATCH();
        do_lpr: /* Load processor register */
            tos0 = psys_spop(s);
            psys_push(s, psys_lpr(s, tos0));
            DISPATCH();
        do_bpt: /* Breakpoint */
            psys_execerror(s, PSYS_ERR_BRKPNT);
            DISPATCH();
        do_bnot: /* Boolean NOT */
            tos0 = psys_pop(s);
            psys_push(s, !BOOL(tos0));
            DISPATCH();
        do_lor: /* Logical OR */
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, tos1 | tos0);
            DISPATCH();
        do_land: /* Logical AND */
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, tos1 & tos0);
            DISPATCH();
        do_adi: /* Add integers */
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, tos1 + tos0);
            DISPATCH();
        do_sbi: /* Subtract integers */
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, tos1 - tos0);
            DISPATCH();
        do_stl: /* Store local */
            arg0 = fetch_V(s);
            tos0 = psys_pop(s);
            psys_stw(s, local_addr(s, arg0), tos0);
            DISPATCH();
        do_sro: /* Store global */
            arg0 = fetch_V(s);
            tos0 = psys_pop(s);
            psys_stw(s, global_addr(s, arg0), tos0);
            DISPATCH();
        do_str: /* Store Intermediate */
            arg0 = fetch_UB(s);
            arg1 = fetch_V(s);
            tos0 = psys_pop(s);
            psys_stw(s, intermd_addr(s, arg0, arg1), tos0);
            DISPATCH();
        do_ldb: /* Load byte */
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, psys_ldb(s, tos1, tos0));
            DISPATCH();
        do_native: /* Enter native code */
            psys_panic("NATIVE not supported");
            return;
        do_nat_info: /* Native code information (skip PC forward over metadata) */
            arg0 = fetch_V(s);
            s->ipc += arg0;
            DISPATCH();
        do_cap: { /* Copy array parameter */ /* segf */
            psys_fulladdr addr;
            arg0 = fetch_V(s);  /* size of array in words */
            tos0 = psys_pop(s); /* address of a parameter descriptor for a packed array of characters */
            tos1 = psys_pop(s); /* destination for the array */
            addr = array_descriptor_to_addr(s, tos0);
            if (addr != PSYS_ADDR_ERROR) {
                memcpy(psys_words(s, tos1), psys_words(s, addr), arg0*2);
            }
            } DISPATCH();
        do_csp: { /* Copy string parameter */ /* segf */
            psys_fulladdr addr;
            arg0 = fetch_UB(s); /* maximum size of string in bytes */
            tos0 = psys_pop(s); /* address of a parameter descriptor for a packed array of characters */
            tos1 = psys_pop(s); /* destination of string */
            addr = array_descriptor_to_addr(s, tos0);
            if (addr != PSYS_ADDR_ERROR) {
                psys_word length = psys_ldb(s, addr, 0);
                if (length > arg0) { /* string doesn't fit */
                    psys_execerror(s, PSYS_ERR_S2LONG);
                } else {
                    if (PDBG(s, STRINGS)) {
                        psys_debug("string: %05x '%.*s' (len %d of %d)\n", addr, length, psys_bytes(s, addr)+1, length, arg0);
                        psys_debug_hexdump(s, addr, length+1);
                    }
                    memcpy(psys_words(s, tos1), psys_words(s, addr), (length/2 + 1)*2);
                }
            }
            } DISPATCH();
        do_slod: /* Short load intermediate */
            arg0 = fetch_V(s);
            psys_push(s, psys_ldw(s, intermd_addr(s, op - PSOP_SLOD1 + 1, arg0)));
            DISPATCH();
        do_equi: /* Equal Integer */
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, tos1 == tos0);
            DISPATCH();
        do_neqi: /* Not Equal Integer */
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, tos1 != tos0);
            DISPATCH();
        do_leqi: /* Less Than or Equal Integer */
            tos0 = psys_spop(s);
            tos1 = psys_spop(s);
            psys_push(s, tos1 <= tos0);
            DISPATCH();
        do_geqi: /* Greater Than or Equal Integer */
            tos0 = psys_spop(s);
            tos1 = psys_spop(s);
            psys_push(s, tos1 >= tos0);
            DISPATCH();
        do_leusw: /* Less Than or Equal Unsigned */
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, tos1 <= tos0);
            DISPATCH();
        do_geusw: /* Greater Than or Equal Unsigned */
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, tos1 >= tos0);
            DISPATCH();
        do_eqpwr: { /* Equal Set (TRUE if all elements match) */
            psys_word *stos0 = psys_stack_words(s, 0);
            psys_word *stos1 = psys_stack_words(s, psys_set_words(stos0));
            psys_pop_n(s, psys_set_words(stos0) + psys_set_words(stos1)); /* drop both sets from stack */
            psys_push(s, psys_set_is_equal(stos1, stos0));
            } DISPATCH();
        do_lepwr: { /* Less Than or Equal Set (TRUE if TOS-1 is a subset of TOS) */
            psys_word *stos0 = psys_stack_words(s, 0);
            psys_word *stos1 = psys_stack_words(s, psys_set_words(stos0));
            psys_pop_n(s, psys_set_words(stos0) + psys_set_words(stos1)); /* drop both sets from stack */
            psys_push(s, psys_set_is_subset(stos1, stos0));
            } DISPATCH();
        do_gepwr: { /* Greater Than or Equal Set (TRUE if TOS-l is a superset of TOS) */
            psys_word *stos0 = psys_stack_words(s, 0);
            psys_word *stos1 = psys_stack_words(s, psys_set_words(stos0));
            psys_pop_n(s, psys_set_words(stos0) + psys_set_words(stos1)); /* drop both sets from stack */
            psys_push(s, psys_set_is_superset(stos1, stos0));
            } DISPATCH();
        do_eqbyte: /* Equal Byte Array */
            arg0 = fetch_UB(s);
            arg1 = fetch_UB(s);
            arg2 = fetch_V(s);
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, compare_bytearrays(s, arg1, tos1, arg0, tos0, arg2) == 0);
            DISPATCH();
        do_lebyte: /* Less Than or Equal Byte Array */
            arg0 = fetch_UB(s);
            arg1 = fetch_UB(s);
            arg2 = fetch_V(s);
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, compare_bytearrays(s, arg1, tos1, arg0, tos0, arg2) <= 0);
            DISPATCH();
        do_gebyte: /* Greater Than or Equal Byte Array */
            arg0 = fetch_UB(s);
            arg1 = fetch_UB(s);
            arg2 = fetch_V(s);
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, compare_bytearrays(s, arg1, tos1, arg0, tos0, arg2) >= 0);
            DISPATCH();
        do_srs: { /* Subrange set */
            psys_set result;
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            if (psys_set_from_subrange(result, tos1, tos0)) {
                psys_set_push(s, result);
            } else {
                psys_execerror(s, PSYS_ERR_SET2LG);
            }
            } DISPATCH();
        do_swap: /* Swap */
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, tos0);
            psys_push(s, tos1);
            DISPATCH();
        do_sto: /* Store - TOS is stored in the word pointed to by TOS-1 */
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_stw(s, tos1, tos0);
            DISPATCH();
        do_mov: { /* Move */
            const psys_word *src;
            psys_word *dst;
            arg0 = fetch_UB(s); /* flag: src from 0) memory or 1)segment 2)segment byteswapped */
            arg1 = fetch_V(s);  /* number of words to copy */
            tos0 = psys_pop(s); /* src addr|ofs */
            tos1 = psys_pop(s); /* dst addr */
            src = psys_words(s, memory_or_segment_addr(s, arg0, tos0));
            dst = psys_words(s, tos1);
#if 0
            psys_debug("MOV: copying 0x%04x words from %05x to %05x\n", arg1, memory_or_segment_addr(s, arg0, tos0), tos1);
#endif
            if (arg0 == 2 && seg_needs_endian_flip(s, s->curseg)) { /* flip bytes if requested and necessary */
                for (x=0; x<arg1; ++x) {
#if 0
                    psys_debug("  %04x\n", psys_flip_endian(src[x]));
#endif
                    dst[x] = psys_flip_endian(src[x]);
                }
            } else {
                for (x=0; x<arg1; ++x) {
#if 0
                    psys_debug("  %04x\n", src[x]);
#endif
                    dst[x] = src[x];
                }
            }
            } DISPATCH();
        do_adj: { /* Adjust set */
            psys_set a;
            arg0 = fetch_UB(s);
            psys_set_pop(s, a);
            if (psys_set_adj(a, arg0)) { /* push set without length word */
                psys_push_n(s, arg0); /* make room for enough words on stack */
                memcpy(psys_stack_words(s, 0), &a[1], arg0*2); /* copy entire structure */
            } else {
                psys_execerror(s, PSYS_ERR_SET2LG);
            }
            } DISPATCH();
        do_stb: /* Store byte */
            tos0 = psys_pop(s); /* Value to write */
            tos1 = psys_pop(s); /* Offset */
            tos2 = psys_pop(s); /* Word address of target */
            psys_stb(s, tos2, tos1, tos0);
            DISPATCH();
        do_ldp: /* Load packed */
            tos0 = psys_pop(s); /* Number of rightmost bit of the field */
            tos1 = psys_pop(s); /* Number of bits in field */
            tos2 = psys_pop(s); /* Address of the word */
            psys_push(s, (psys_ldw(s, tos2) >> tos0) & (BIT(tos1)-1));
            DISPATCH();
        do_stp: { /* Store packed */
            unsigned mask;
            tos0 = psys_pop(s); /* Value to store */
            tos1 = psys_pop(s); /* Number of rightmost bit of the field */
            tos2 = psys_pop(s); /* Number of bits in field */
            tos3 = psys_pop(s); /* Address of the word */
            mask = (BIT(tos2)-1) << tos1;
            psys_stw(s, tos3, (psys_ldw(s, tos3) & ~mask) | ((tos0 << tos1) & mask));
            } DISPATCH();
        do_chk: /* Check subrange bounds */
            tos0 = psys_spop(s);
            tos1 = psys_spop(s);
            tos2 = psys_spop(s);
            if (tos2 < tos1 || tos2 > tos0) {
                psys_execerror(s, PSYS_ERR_INVNDX);
            } else {
                psys_push(s, tos2);
            }
            DISPATCH();
        do_ldm: /* Load multiple */
            arg0 = fetch_UB(s);
            tos0 = psys_pop(s);
            /* push in reversed order because the words should appear in the same order
             * on the stack as in memory.
             */
            for (x=arg0-1; x>=0; --x) {
                psys_push(s, psys_ldw(s, W(tos0, x)));
            }
            DISPATCH();
        do_spr: /* Store processor register */
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_spr(s, tos1, tos0);
            DISPATCH();
        do_efj: /* Equal false jump */
            arg0 = fetch_SB(s);
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            if (tos1 != tos0) {
                s->ipc += arg0;
            }
            DISPATCH();
        do_nfj: /* Not equal false jump */
            arg0 = fetch_SB(s);
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            if (tos1 == tos0) {
                s->ipc += arg0;
            }
            DISPATCH();
        do_fjp: /* False jump */
            arg0 = fetch_SB(s);
            tos0 = psys_pop(s);
            if (!BOOL(tos0)) {
                s->ipc += arg0;
            }
            DISPATCH();
        do_fjpl: /* False jump long */
            arg0 = fetch_W(s);
            tos0 = psys_pop(s);
            if (!BOOL(tos0)) {
                s->ipc += arg0;
            }
            DISPATCH();
        do_xjp: { /* Case jump */
            int addr, b, e;
            bool flip = seg_needs_endian_flip(s, s->curseg);
            arg0 = fetch_V(s);
            tos0 = psys_spop(s);
            addr = s->curseg + seg_cpool_ofs(s, s->curseg, arg0);
            b = psys_ldsw_flip(s, W(addr, 0), flip);
            e = psys_ldsw_flip(s, W(addr, 1), flip);
            if (tos0 >= b && tos0 <= e)
            {
                s->ipc += psys_ldsw_flip(s, W(addr, 2 + tos0 - b), flip);
            }
            } DISPATCH();
        do_ixa: /* Index array */
            arg0 = fetch_V(s);
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, W(tos1, arg0*tos0));
            DISPATCH();
        do_ixp: /* Index packed array */
            arg0 = fetch_UB(s);
            arg1 = fetch_UB(s);
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, W(tos1, tos0 / arg0)); /* Address of the word */
            psys_push(s, arg1);                 /* Number of bits in field */
            psys_push(s, (tos0 % arg0) * arg1); /* Number of rightmost bit of the field */
            DISPATCH();
        do_ste: { /* Store extended */
            psys_fulladdr addr;
            arg0 = fetch_UB(s);
            arg1 = fetch_V(s);
            tos0 = psys_pop(s);
            addr = extended_addr(s, arg0, arg1);
            if (addr) { /* continue only if segment could be found */
                psys_stw(s, addr, tos0);
            }
            } DISPATCH();
        do_inn: { /* Set membership */
            /* Note: arguments order is reversed compared to p-system reference:
             * set is on the top of the stack, the element to check membership of is below that.
             */
            psys_word *data = psys_stack_words(s, 0); /* set length|set|tos1 */
            psys_word ofs = psys_set_words(data);
            /* overwrite input word on stack */
            psys_stw(s, W(s->sp, ofs), psys_set_in(data, psys_ldw(s, W(s->sp, ofs))));
            psys_pop_n(s, ofs); /* drop set size and set */
            } DISPATCH();
        do_uni: { /* Set union (bitwise OR) */
            psys_set result;
            psys_word *stos0 = psys_stack_words(s, 0);
            psys_word *stos1 = psys_stack_words(s, psys_set_words(stos0));
            if (psys_set_union(result, stos1, stos0)) {
                psys_pop_n(s, psys_set_words(stos0) + psys_set_words(stos1)); /* drop both sets from stack */
                psys_set_push(s, result); /* push result */
            } else {
                psys_execerror(s, PSYS_ERR_SET2LG);
            }
            } DISPATCH();
        do_int: { /* Set intersection (bitwise AND) */
            psys_set result;
            psys_word *stos0 = psys_stack_words(s, 0);
            psys_word *stos1 = psys_stack_words(s, psys_set_words(stos0));
            if (psys_set_intersection(result, stos1, stos0)) {
                psys_pop_n(s, psys_set_words(stos0) + psys_set_words(stos1)); /* drop both sets from stack */
                psys_set_push(s, result); /* push result */
            } else {
                psys_execerror(s, PSYS_ERR_SET2LG);
            }
            } DISPATCH();
        do_dif: { /* Set difference (TOS-1 AND NOT TOS) */
            psys_set result;
            psys_word *stos0 = psys_stack_words(s, 0);
            psys_word *stos1 = psys_stack_words(s, psys_set_words(stos0));
            if (psys_set_difference(result, stos1, stos0)) {
                psys_pop_n(s, psys_set_words(stos0) + psys_set_words(stos1)); /* drop both sets from stack */
                psys_set_push(s, result); /* push result */
            } else {
                psys_execerror(s, PSYS_ERR_SET2LG);
            }
            } DISPATCH();
        do_signal: /* Signal */ /* segf */
            tos0 = psys_pop(s);
            psys_signal(s, tos0, true);
            DISPATCH();
        do_wait: /* Wait */ /* segf */
            tos0 = psys_pop(s);
            psys_wait(s, tos0);
            DISPATCH();
        do_abi: /* Absolute value integer */
            tos0 = psys_spop(s);
            psys_push(s, abs(tos0));
            DISPATCH();
        do_ngi: /* Negate integer */
            tos0 = psys_spop(s);
            psys_push(s, -tos0);
            DISPATCH();
        do_dup1: /* Duplicate one word */
            tos0 = psys_pop(s);
            psys_push(s, tos0);
            psys_push(s, tos0);
            DISPATCH();
        do_lnot: /* Logical NOT */
            tos0 = psys_pop(s);
            psys_push(s, ~tos0);
            DISPATCH();
        do_ind: /* Index */
            arg0 = fetch_V(s);
            tos0 = psys_pop(s);
            psys_push(s, psys_ldw(s, W(tos0, arg0)));
            DISPATCH();
        do_inc: /* Increment */
            arg0 = fetch_V(s);
            tos0 = psys_pop(s);
            psys_push(s, W(tos0, arg0));
            DISPATCH();
        do_eqstr: /* Equal string */
            arg0 = fetch_UB(s);
            arg1 = fetch_UB(s);
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, compare_strings(s, arg1, tos1, arg0, tos0) == 0);
            DISPATCH();
        do_lestr: /* Less or equal string */
            arg0 = fetch_UB(s);
            arg1 = fetch_UB(s);
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, compare_strings(s, arg1, tos1, arg0, tos0) <= 0);
            DISPATCH();
        do_gestr: /* Greater or equal string */
            arg0 = fetch_UB(s);
            arg1 = fetch_UB(s);
            tos0 = psys_pop(s);
            tos1 = psys_pop(s);
            psys_push(s, compare_strings(s, arg1, tos1, arg0, tos0) >= 0);
            DISPATCH();
        do_astr: { /* Assign string */
            const psys_byte *src;
            psys_byte *dst;
            arg0 = fetch_UB(s); /* flag: src from memory or segment */
            arg1 = fetch_UB(s); /* decared size of destination */
            tos0 = psys_pop(s); /* src addr|ofs */
            tos1 = psys_pop(s); /* dst addr */
            src = psys_bytes(s, memory_or_segment_addr(s, arg0, tos0));
            dst = psys_bytes(s, tos1);
            if (src[0] > arg1) { /* source is larger than destination */
                psys_execerror(s, PSYS_ERR_S2LONG);
            } else { /* copy string and length byte */
                memcpy(dst, src, src[0]+1);
            }
            } DISPATCH();
        do_cstr: /* Check string index */
            tos0 = psys_pop(s); /* index into variable */
            tos1 = psys_pop(s); /* address of string variable */
            if (tos0 < 1 || tos0 > psys_ldb(s, tos1, 0)) {
                psys_execerror(s, PSYS_ERR_INVNDX);
            } else {
                psys_push(s, tos1);
                psys_push(s, tos0);
            }
            DISPATCH();
        do_inci: /* Increase integer */
            tos0 = psys_pop(s);
            psys_push(s, tos0 + 1);
            DISPATCH();
        do_deci: /* Decrease integer */
            tos0 = psys_pop(s);
            psys_push(s, tos0 - 1);
            DISPATCH();
        do_scip: /* Short call intermediate procedure */
            arg0 = fetch_UB(s);
            handle_call(s, CALL_CURSEG, op - PSOP_SCIP1 + 1, arg0);
            DISPATCH();
        do_tjp: /* True jump */
            arg0 = fetch_SB(s);
            tos0 = psys_pop(s);
            if (BOOL(tos0)) {
                s->ipc += arg0;
            }
            DISPATCH();
        /* Floating point ops - not implemented */
        do_flt: /* Float */
        do_eqreal: /* Equal Real */
        do_lereal: /* Less Than or Equal Real */
        do_gereal: /* Greater Than or Equal Real */
        do_dup2: /* Duplicate Real */
        do_abr: /* Absolute Real */
        do_ngr: /* Negate Real */
        do_ldcrl: /* Load Constant Real */
        do_ldrl: /* Load Real */
        do_strl: /* Store Real */
            psys_execerror(s, PSYS_ERR_FPIERR);
            DISPATCH();
        do_invalid:
            psys_execerror(s, PSYS_ERR_NOTIMP);
            DISPATCH();
    }
    // clang-format on
}

void psys_execerror(struct psys_state *s, psys_word err)
{
    /* restore processor state from before last instruction */
    s->sp      = s->stored_sp;
    s->ipc     = s->stored_ipc;
    s->running = false;
    /* TODO proper error handling. The PME is supposed to push the
     * current CPU state on the stack then call kernel function
     * 0x02 (Exec_Error) for error handling.
     */
    if (PDBG(s, WARNING)) {
        psys_debug("Execution error %d (fatal)\n", err);
        psys_print_traceback(s);
    }
}

void psys_fault(struct psys_state *s, psys_fulladdr tib, psys_fulladdr erec, psys_fulladdr words, psys_word type)
{
    if (PDBG(s, WARNING)) {
        psys_debug("Execution fault %d: invoking fault handler task\n", type);
        psys_print_traceback(s);
    }
    /* restore processor state from before last instruction */
    s->sp  = s->stored_sp;
    s->ipc = s->stored_ipc;
    psys_stw(s, s->syscom + PSYS_SYSCOM_FAULT_TIS, s->curtask);
    psys_stw(s, s->syscom + PSYS_SYSCOM_FAULT_EREC, erec);
    psys_stw(s, s->syscom + PSYS_SYSCOM_FAULT_WORDS, words);
    psys_stw(s, s->syscom + PSYS_SYSCOM_FAULT_TYPE, type);
    /* raise signal so that fault handler task can pick up the pieces
     * and put humpty-dumpty back together again. */
    psys_signal(s, s->syscom + PSYS_SYSCOM_REAL_SEM, true);
}

void psys_stop(struct psys_state *s)
{
    s->running = false;
}
