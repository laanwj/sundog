/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
/* p-system Run-time Support Package.
 * This provides OS functions to handle procedure calls on segment 1.
 * These are also called "standard procedures" in some texts.
 */
#include "psys_rsp.h"

#include "psys_constants.h"
#include "psys_debug.h"
#include "psys_helpers.h"
#include "psys_state.h"
#include "psys_task.h"
#include "util/memutil.h"
#include "util/util_minmax.h"
#include "util/util_save_state.h"

#include <string.h>
#include <time.h>
#include <unistd.h>

/* Header for savestates */
#define PSYS_RSP_STATE_ID 0x50525350

/** Internal helper for setting IORESULT register. */
static void psys_set_io_result(struct psys_state *state, psys_word result)
{
    psys_stw(state, state->syscom + PSYS_SYSCOM_IORSLT, result);
}

/** RELOCSEG relocates the segment pointed to by the ERec.
 *
 * In this implementation, no relocation is performed at all because native code is not supported.
 */
static void psys_rsp_relocseg(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_word erec = psys_pop(state);
    if (PDBG(state, RSP)) {
        psys_debug("relocseg 0x%04x (stub)\n", erec);
    }
    /* Nothing to do here - no native code support */
}

/** moveseg(sib,srcpool,srcoffset)
 *
 * MOVESEG moves the segment at offset TOS in the pool described by TOS-1 to
 * the location specified in the SIB pointed to by TOS-2, and relocates it.
 * Only segment-relative relocation is performed.
 *
 * In this implementation, no relocation is performed at all because native code is not supported.
 */
static void psys_rsp_moveseg(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_word srcoffset       = psys_pop(state);
    psys_word srcpool         = psys_pop(state);
    psys_word sib             = psys_pop(state);
    psys_fulladdr srcpoolbase = psys_pool_get_base(state, srcpool);
    psys_word dstpool         = psys_ldw(state, sib + PSYS_SIB_Seg_Pool);
    psys_word dstoffset       = psys_ldw(state, sib + PSYS_SIB_Seg_Base);
    psys_word len             = psys_ldw(state, sib + PSYS_SIB_Seg_Leng);
    psys_fulladdr dstpoolbase = psys_pool_get_base(state, dstpool);
    if (PDBG(state, RSP)) {
        psys_debug("moveseg %04x %04x(%05x):%04x <- %04x(%05x):%04x (0x%04x words)\n",
            sib,
            dstpool, dstpoolbase, dstoffset,
            srcpool, srcpoolbase, srcoffset,
            len);
    }
    if (dstoffset == 0) {
        psys_panic("moveseg destination segment not resident\n");
    }
    memmove(psys_bytes(state, dstpoolbase) + dstoffset,
        psys_bytes(state, srcpoolbase) + srcoffset,
        len * 2);
}

/** moveleft(source,dest:array; length:integer)
 *
 * Move bytes one at a time starting from the left (low order byte).
 */
static void psys_rsp_moveleft(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_sword length     = psys_spop(state);
    psys_word dest_ofs    = psys_pop(state);
    psys_word dest_base   = psys_pop(state);
    psys_word source_ofs  = psys_pop(state);
    psys_word source_base = psys_pop(state);
    psys_byte *dest, *source;
    int x;
    if (PDBG(state, RSP)) {
        psys_debug("moveleft((0x%04x,0x%04x)=0x%04x,(0x%04x,0x%04x)=0x%04x,0x%04x)\n",
            source_base, source_ofs, source_base + source_ofs,
            dest_base, dest_ofs, dest_base + dest_ofs,
            length);
    }
    source = psys_bytes(state, source_base + source_ofs);
    dest   = psys_bytes(state, dest_base + dest_ofs);
    for (x = 0; x < length; ++x) {
        dest[x] = source[x];
    }
}

/** moveright(source,dest:array; length:integer)
 *
 * Move bytes one at a time starting from the right (high order byte).
 */
static void psys_rsp_moveright(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_sword length     = psys_spop(state);
    psys_word dest_ofs    = psys_pop(state);
    psys_word dest_base   = psys_pop(state);
    psys_word source_ofs  = psys_pop(state);
    psys_word source_base = psys_pop(state);
    psys_byte *dest, *source;
    int x;
    if (PDBG(state, RSP)) {
        psys_debug("moveleft((0x%04x,0x%04x),(0x%04x,0x%04x),0x%04x)\n",
            source_base, source_ofs, dest_base, dest_ofs, length);
    }
    source = psys_bytes(state, source_base + source_ofs);
    dest   = psys_bytes(state, dest_base + dest_ofs);
    for (x = length - 1; x >= 0; --x) {
        dest[x] = source[x];
    }
}

/** Merged i/o function - internal logic for both unit_read and unit_write */
static psys_word unitrw(struct psys_state *state, struct psys_rsp_state *rsp, bool wr, psys_word unit, psys_fulladdr buf_addr, psys_word len, psys_word block, psys_word ctrl)
{
    psys_byte *buf = psys_bytes(state, buf_addr);
    switch (unit) {
    case PSYS_UNIT_SYSTEM:
        /* Reading or writing to SYS stops the machine */
        psys_panic("Halted\n");
        break;
    case PSYS_UNIT_CONSOLE: {
        if (wr) {
            int ret = write(STDOUT_FILENO, buf, len);
            (void)ret;
        } else {
            int ret = read(STDIN_FILENO, buf, len);
            (void)ret;
        }
    } break;
    case PSYS_UNIT_DISK0: {
        unsigned x, srcblk;
        if (ctrl & 2) { /* physical addressing starts a second earlier */
            len    = 512;
            srcblk = block;
        } else {
            srcblk = block + rsp->disk0_track;
        }
        if (!rsp->disk0)
            psys_panic("disk0 not mounted\n");
        for (x = 0; x < len; x += PSYS_BLOCK_SIZE, srcblk += 1) { /* copy per block, wrapping as necessary */
            unsigned remainder = umin(len - x, PSYS_BLOCK_SIZE);
#if 0
            unsigned y = 0;
#endif
            if (srcblk > rsp->disk0_size) {
                if (!rsp->disk0_wrap) {
                    psys_panic("block out of range and wrap disabled");
                }
                srcblk %= rsp->disk0_size;
            }
            if (PDBG(state, RSP)) {
                psys_debug("%s 0x%04x 0x%04x 0x%04x\n", wr ? "write" : "read", buf_addr + x, srcblk * PSYS_BLOCK_SIZE, remainder);
            }
            if (wr) {
                memcpy(rsp->disk0 + srcblk * PSYS_BLOCK_SIZE, buf + x, remainder);
            } else {
                memcpy(buf + x, rsp->disk0 + srcblk * PSYS_BLOCK_SIZE, remainder);
#if 0
                psys_debug("  ");
                for (y=0; y<remainder; ++y) {
                    psys_debug("0x%02x ", buf[x+y]);
                }
#endif
            }
            if (PDBG(state, RSP)) {
                psys_debug("\n");
            }
        }
    } break;
    default:
        psys_panic("I/O not implemented for unit %d\n", unit);
    }
    return PSYS_IO_NOERROR;
}

/** unitread(unit:integer; buf:array; len,block,ctrl:integer)
 *
 * Read from unit (device).
 */
static void psys_rsp_unitread(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_word ctrl     = psys_pop(state);
    psys_word block    = psys_pop(state);
    psys_word len      = psys_pop(state);
    psys_word buf_ofs  = psys_pop(state);
    psys_word buf_base = psys_pop(state);
    psys_word unit     = psys_pop(state);
    if (PDBG(state, RSP)) {
        psys_debug("unitread(%i, (0x%04x, 0x%04x), 0x%x, 0x%x, 0x%x)\n",
            unit, buf_base, buf_ofs, len, block, ctrl);
    }
    psys_set_io_result(state,
        unitrw(state, rsp, false, unit, buf_base + buf_ofs, len, block, ctrl));
}

/** unitwrite(unit:integer; buf:array; len,block,ctrl:integer)
 *
 * Write to unit (device).
 */
static void psys_rsp_unitwrite(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_word ctrl     = psys_pop(state);
    psys_word block    = psys_pop(state);
    psys_word len      = psys_pop(state);
    psys_word buf_ofs  = psys_pop(state);
    psys_word buf_base = psys_pop(state);
    psys_word unit     = psys_pop(state);
    if (PDBG(state, RSP)) {
        psys_debug("unitwrite(%i, (0x%04x, 0x%04x), 0x%x, 0x%x, 0x%x)\n",
            unit, buf_base, buf_ofs, len, block, ctrl);
    }
    psys_set_io_result(state,
        unitrw(state, rsp, true, unit, buf_base + buf_ofs, len, block, ctrl));
}

/** time(hiword,loword)
 *
 * TIME saves the high and low words of the system clock (a 32-bit 60 Hz clock)
 * in the indicated words.
 */
static void psys_rsp_time(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_word loword = psys_pop(state);
    psys_word hiword = psys_pop(state);
    uint32_t time;
#ifdef PSYS_ACTUAL_TIME
    struct timespec tp;
#endif

    if (PDBG(state, RSP)) {
        psys_debug("time %04x %04x\n", hiword, loword);
    }
#ifdef PSYS_ACTUAL_TIME
    clock_gettime(CLOCK_MONOTONIC, &tp);
    time = tp.tv_sec * 60 + tp.tv_nsec / 16666667;
#else
    time = rsp->time;
#endif

    psys_stw(state, hiword, time >> 16);
    psys_stw(state, loword, time & 0xffff);
}

/** fillchar(dest:bytearray; n_bytes,value:integer)
 *
 * FILLCHAR fills a range of memory with a single-byte value.
 * TOS is the character. TOS-1 is the length to fill. TOS-2 is the starting
 * address for the fill. If TOS-1 is zero or negative, no filling is done.
 * Otherwise, memory is filled with the byte TOS for TOS-1 bytes starting at
 * address TOS-2.
 */
static void psys_rsp_fillchar(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_word value     = psys_pop(state);
    psys_sword n_bytes  = psys_spop(state);
    psys_word dest_ofs  = psys_pop(state);
    psys_word dest_base = psys_pop(state);
    psys_byte *dest;
    if (PDBG(state, RSP)) {
        psys_debug("fillchar (0x%04x,0x%04x) 0x%04x %04x\n", dest_base, dest_ofs, n_bytes, value);
    }
    dest = psys_bytes(state, dest_base + dest_ofs);
    if (n_bytes > 0) {
        memset(dest, value, n_bytes);
    }
}

/** scan(len,exp,source):int
 *
 * Scan memory forward or backward for a certain character. TOS is a mask
 * field (unused). TOS-1 is a pointer to the array to scan. TOS-2 is the byte
 * to look for. TOS-3 is the scan kind (0 means until equal, 1 means until not
 * equal). TOS-4 is the length to scan. If TOS-4 is negative, the scan proceeds
 * to the left. TOS-S is the function result word.
 */
static void psys_rsp_scan(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_word mask        = psys_pop(state);
    psys_word source_ofs  = psys_pop(state);
    psys_word source_base = psys_pop(state);
    psys_word needle      = psys_pop(state);
    psys_word kind        = psys_pop(state);
    psys_sword len        = psys_pop(state);
    psys_word i           = 0;
    psys_sword direction;
    if (PDBG(state, RSP)) {
        psys_debug("scan %d %d 0x%02x (0x%04x,0x%04x)=%04x 0x%04x\n",
            len, kind, needle, source_base, source_ofs, source_base + source_ofs, mask);
    }
    /* Positive length means scan forward, negative scan means to scan backward */
    direction = len < 0 ? -1 : 1;
    len       = abs(len);
    /* Scan until character 'needle' is matched, or not matched,
     * depending on kind. */
    switch (kind) {
    case 0: /* until equal */
        while (i < len && psys_ldb(state, source_base, source_ofs) != needle) {
            i += 1;
            source_ofs += direction;
        }
        break;
    case 1: /* until not equal */
        while (i < len && psys_ldb(state, source_base, source_ofs) == needle) {
            i += 1;
            source_ofs += direction;
        }
        break;
    default:
        psys_panic("Unknown scan kind %d\n", kind);
    }
    /* Write return value */
    psys_stw(state, state->sp, i);
}

/** iocheck()
 *
 * IOCHECK tests the p-machine register IORESULT for zero. If the register is
 * nonzero, an I/O execution error is issued.
 */
static void psys_rsp_iocheck(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_panic("iocheck not implemented\n");
}

/** getpoolbytes(dest,pooldesc,offset,nbytes)
 *
 * GETPOOLBYTES get TOS bytes from the pool described by TOS-2 at offset TOS-1,
 * and places them at TOS-3.
 */
static void psys_rsp_getpoolbytes(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_sword n_bytes  = psys_spop(state);
    psys_word offset   = psys_pop(state);
    psys_word pooldesc = psys_pop(state);
    psys_word dest     = psys_pop(state);
    psys_fulladdr poolbase;
    poolbase = psys_pool_get_base(state, pooldesc);
    if (PDBG(state, RSP)) {
        psys_debug("getpoolbytes %04x %04x (base %05x) %04x %04x\n", dest, pooldesc, poolbase, offset, n_bytes);
    }
    if (n_bytes > 0) {
        memcpy(psys_bytes(state, dest), psys_bytes(state, poolbase) + offset, n_bytes);
    }
}

/** putpoolbytes(...).
 *
 * PUTPOOLBYTES writes TOS bytes from the buffer TOS-3 to the pool described by
 * TOS-2 at offset TOS-l.
 */
static void psys_rsp_putpoolbytes(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_panic("putpoolbytes not implemented\n");
}

/** flipsegbytes(erec,offset,nwords)
 *
 * FLIPSEGBYTES flips TOS words starting at word offset TOS-1 in the segment
 * described by TOS-2.
 */
static void psys_rsp_flipsegbytes(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_sword nwords = psys_spop(state);
    psys_word offset  = psys_pop(state);
    psys_word erec    = psys_pop(state);
    psys_fulladdr segbase;
    psys_word *buf;
    if (PDBG(state, RSP)) {
        psys_debug("flipsegbytes %04x %04x %04x\n", erec, offset, nwords);
    }

    segbase = psys_segment_from_erec(state, erec, true);
    if (segbase == PSYS_ADDR_ERROR) {
        psys_panic("flipsegbytes on non-resident segment");
    }
    buf = psys_words(state, W(segbase, offset));
    for (int x = 0; x < nwords; ++x) {
        buf[x] = psys_flip_endian(buf[x]);
    }
}

/** quiet()
 *
 * QUIET must disable all p-machine events such that no attached semaphore is
 * signalled until the corresponding call to ENABLE is made.
 */
static void psys_rsp_quiet(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    rsp->events_enabled = true;
}

/** enable()
 *
 * ENABLE reenables p-machine events that have been disabled by QUIET.
 */
static void psys_rsp_enable(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    rsp->events_enabled = false;
}

/** attach(semaphore,vector)
 *
 * TOS is the number of a p-machine event vector. It must be in the range 0
 * through 63. TOS-1 is the address of a semaphore.
 *
 * ATTACH associates the semaphore pointed to by TOS-l with the vector TOS such
 * that whenever the event TOS is recognized, the semaphore is signaled. If
 * the semaphore pointer is NIL, vector TOS must be unattached from any
 * sempahore it was formerly attached to. If TOS isn't in the range 0 through
 * 63, no operation is performed.
 */
static void psys_rsp_attach(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_word vector    = psys_pop(state);
    psys_word semaphore = psys_pop(state);
    if (PDBG(state, RSP)) {
        psys_debug("attach 0x%04x 0x%04x\n", semaphore, vector);
    }
    if (vector < PSYS_MAX_EVENTS) {
        rsp->events[vector] = semaphore;
    }
}

/** ioresult(): integer
 *
 * IORESULT returns the value of the p-machine register IORESULT.
 */
static void psys_rsp_ioresult(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    if (PDBG(state, RSP)) {
        psys_debug("ioresult()\n");
    }
    psys_stw(state, state->sp, psys_ldw(state, state->syscom + PSYS_SYSCOM_IORSLT));
}

/** unitbusy(unit): boolean
 *
 * UNITBUSY returns TRUE if there is any outstanding I/O on device TOS, and
 * FALSE otherwise. On return, IORESULT contains status information.
 * This is unused as asynchronous I/O for the p-machine was never defined.
 */
static void psys_rsp_unitbusy(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_panic("unitbusy not implemented\n");
}

/** poweroften(power): real
 *
 * Floating-point power of ten.
 */
static void psys_rsp_poweroften(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_panic("poweroften not implemented\n");
}

/** unitwait(...)
 *
 * The p-machine waits until all I/O on unit TOS is completed. On return,
 * IORESULT contains status information.
 * This is unused as asynchronous I/O for the p-machine was never defined.
 */
static void psys_rsp_unitwait(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_panic("unitwait not implemented\n");
}

/** unitclear(unit)
 *
 * The device with unit number TOS is initialized to its "power-up" state. On
 * return, IORESULT contains status information.
 */
static void psys_rsp_unitclear(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_word unit   = psys_pop(state);
    psys_word result = PSYS_IO_NOERROR;
    if (PDBG(state, RSP)) {
        psys_debug("unitclear(%d) (stub)\n", unit);
    }
    switch (unit) {
    case 3: /* ? */
        result = PSYS_IO_BADMODE;
        break;
    case 20: /* ? */
    case 21:
    case 22:
        result = PSYS_IO_NOUNIT;
        break;
    }
    psys_set_io_result(state, result);
}

/** unitstatus(unit,stat_rec,control)
 */
static void psys_rsp_unitstatus(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_word control  = psys_pop(state);
    psys_word stat_rec = psys_pop(state);
    psys_word unit     = psys_pop(state);
    psys_word result   = PSYS_IO_NOERROR;
    if (PDBG(state, RSP)) {
        psys_debug("unitstatus(%d,0x%04x,0x%x)", unit, stat_rec, control);
    }
    switch (unit) {
    case 0x80: { /* Sundog specific - what is this? */
        uint32_t addr1 = 0x00018bac;
        uint32_t addr2 = 0x00018876;
        psys_stw(state, W(stat_rec, 0), addr1 >> 16);
        psys_stw(state, W(stat_rec, 1), addr1 & 0xffff);
        psys_stw(state, W(stat_rec, 2), addr2 >> 16);
        psys_stw(state, W(stat_rec, 3), addr2 & 0xffff);
        psys_stw(state, W(stat_rec, 4), 0x0000); /* Game disk ST drive number (A=0, B=1, ...) */
    } break;
    default:
        psys_panic("unitstatus not implemented for unit %d\n", unit);
    }
    psys_set_io_result(state, result);
}

/** idsearch(...)
 *
 * PASCAL reserved identifier search. Used by compiler only.
 */
static void psys_rsp_idsearch(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_panic("idsearch not implemented\n");
}

/** treesearch(root,foundp,target):int
 *
 * TREESEARCH searches the symbol table tree TOS-2 for the target string TOS,
 * returning a pointer to where the target was found in the variable pointed to
 * by TOS-l. If the target wasn't found, the variable pointed to by TOS-1 will
 * point to the leaf node of the tree that was searched last.
 */
static void psys_rsp_treesearch(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_word target = psys_pop(state);
    psys_word foundp = psys_pop(state);
    psys_word root   = psys_pop(state);
    psys_word node, lastnode;
    int cmp = 0, retval;
    /* node format:
     * 0   name (8 characters)
     * 8   right link (pointer)
     * 10  left link (pointer)
     */
    if (PDBG(state, RSP)) {
        psys_debug("treesearch 0x%04x 0x%04x 0x%04x\n", root, foundp, target);
        psys_debug_hexdump(state, target, 8);
        psys_debug_hexdump(state, root, 12);
    }

    node     = root;
    lastnode = PSYS_NIL;
    while (node != PSYS_NIL) {
        cmp      = memcmp(psys_bytes(state, target), psys_bytes(state, node), 8);
        lastnode = node;
        if (cmp == 0) { /* found! */
            break;
        }
        if (cmp < 0) { /* to the left */
            node = psys_ldw(state, W(node, 5));
        }
        if (cmp > 0) { /* to the right */
            node = psys_ldw(state, W(node, 4));
        }
    }

    psys_stw(state, foundp, lastnode);
    /* return value based on last comparison:
     * 0   target was found
     * -1  target is to the left
     * 1   target is to the right
     */
    if (cmp == 0) {
        retval = 0;
    } else if (cmp < 0) {
        retval = -1;
    } else /* if (cmp > 0) */ {
        retval = 1;
    }
    psys_stw(state, state->sp, retval);
}

/** readseg(erec): integer
 *
 * READSEG reads the segment described by TOS into memory at the location
 * described in the SIB. The completion code in p-machine register IORESULT is
 * returned as the function result.
 */
static void psys_rsp_readseg(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_fulladdr sib, vip, block, segpool, segbase, poolbase, seglen;
    psys_word result;
    psys_word erec = psys_pop(state);
    if (PDBG(state, RSP)) {
        psys_debug("readseg(0x%x)\n", erec);
    }
    sib      = psys_ldw(state, erec + PSYS_EREC_Env_SIB);
    vip      = psys_ldw(state, sib + PSYS_SIB_Vol_Info);
    block    = psys_ldw(state, sib + PSYS_SIB_Seg_Addr); /* disk offset in blocks */
    segpool  = psys_ldw(state, sib + PSYS_SIB_Seg_Pool);
    seglen   = psys_ldw(state, sib + PSYS_SIB_Seg_Leng);
    segbase  = psys_ldw(state, sib + PSYS_SIB_Seg_Base);
    poolbase = psys_pool_get_base(state, segpool);
    if (PDBG(state, RSP)) {
        psys_debug("read segment %.8s sib=0x%04x vip=0x%04x block 0x%04x to 0x%08x:0x%04x (maxlen 0x%04x)\n", psys_bytes(state, sib + PSYS_SIB_Seg_Name), sib, vip, block,
            poolbase, segbase, seglen);
    }
    /* TODO figure out VIP structure - for now, always read from disk 0 */
    result = unitrw(state, rsp, false, PSYS_UNIT_DISK0, poolbase + segbase, seglen * 2, block, 0);

    /* write return value (i/o result) */
    psys_set_io_result(state, result);
    psys_stw(state, state->sp, result);
}

/** setrestricted(flag)
 *
 * ???
 */
static void psys_rsp_setrestricted(struct psys_state *state, struct psys_rsp_state *rsp, psys_fulladdr segment, psys_fulladdr env_data)
{
    psys_panic("setrestricted not implemented\n");
}

static int psys_rsp_save_state(struct psys_binding *b, int fd)
{
    struct psys_rsp_state *rsp = (struct psys_rsp_state *)b->userdata;
    uint32_t id                = PSYS_RSP_STATE_ID;
    if (FD_WRITE(fd, id)
        || FD_WRITE(fd, rsp->disk0_size)
        || FD_WRITE(fd, rsp->disk0_track)
        || FD_WRITE(fd, rsp->disk0_wrap)
        || FD_WRITE(fd, rsp->events_enabled)
        || FD_WRITE(fd, rsp->events)
        || FD_WRITE(fd, rsp->time)) {
        return -1;
    }
    if (write(fd, rsp->disk0, rsp->disk0_size) < (ssize_t)rsp->disk0_size) {
        return -1;
    }
    return 0;
}

static int psys_rsp_load_state(struct psys_binding *b, int fd)
{
    struct psys_rsp_state *rsp = (struct psys_rsp_state *)b->userdata;
    uint32_t id;
    if (FD_READ(fd, id)) {
        return -1;
    }
    if (id != PSYS_RSP_STATE_ID) {
        psys_debug("Invalid psys rsp state record %08x\n", id);
        return -1;
    }

    if (FD_READ(fd, rsp->disk0_size)
        || FD_READ(fd, rsp->disk0_track)
        || FD_READ(fd, rsp->disk0_wrap)
        || FD_READ(fd, rsp->events_enabled)
        || FD_READ(fd, rsp->events)
        || FD_READ(fd, rsp->time)) {
        return -1;
    }
    if (read(fd, rsp->disk0, rsp->disk0_size) < (ssize_t)rsp->disk0_size) {
        return -1;
    }
    return 0;
}

struct psys_binding *psys_new_rsp(struct psys_state *state)
{
    struct psys_binding *b     = CALLOC_STRUCT(psys_binding);
    struct psys_rsp_state *rsp = CALLOC_STRUCT(psys_rsp_state);

    rsp->psys           = state;
    rsp->events_enabled = true;

    b->userdata     = rsp;
    b->handlers     = calloc(PSYS_RSP_COUNT, sizeof(psys_bindingfunc *));
    b->num_handlers = PSYS_RSP_COUNT;

    b->handlers[PSYS_RSP_RELOCSEG]     = (psys_bindingfunc *)&psys_rsp_relocseg;
    b->handlers[PSYS_RSP_MOVESEG]      = (psys_bindingfunc *)&psys_rsp_moveseg;
    b->handlers[PSYS_RSP_MOVELEFT]     = (psys_bindingfunc *)&psys_rsp_moveleft;
    b->handlers[PSYS_RSP_MOVERIGHT]    = (psys_bindingfunc *)&psys_rsp_moveright;
    b->handlers[PSYS_RSP_UNITREAD]     = (psys_bindingfunc *)&psys_rsp_unitread;
    b->handlers[PSYS_RSP_UNITWRITE]    = (psys_bindingfunc *)&psys_rsp_unitwrite;
    b->handlers[PSYS_RSP_TIME]         = (psys_bindingfunc *)&psys_rsp_time;
    b->handlers[PSYS_RSP_FILLCHAR]     = (psys_bindingfunc *)&psys_rsp_fillchar;
    b->handlers[PSYS_RSP_SCAN]         = (psys_bindingfunc *)&psys_rsp_scan;
    b->handlers[PSYS_RSP_IOCHECK]      = (psys_bindingfunc *)&psys_rsp_iocheck;
    b->handlers[PSYS_RSP_GETPOOLBYTES] = (psys_bindingfunc *)&psys_rsp_getpoolbytes;
    b->handlers[PSYS_RSP_PUTPOOLBYTES] = (psys_bindingfunc *)&psys_rsp_putpoolbytes;
    b->handlers[PSYS_RSP_FLIPSEGBYTES] = (psys_bindingfunc *)&psys_rsp_flipsegbytes;
    b->handlers[PSYS_RSP_QUIET]        = (psys_bindingfunc *)&psys_rsp_quiet;
    b->handlers[PSYS_RSP_ENABLE]       = (psys_bindingfunc *)&psys_rsp_enable;
    b->handlers[PSYS_RSP_ATTACH]       = (psys_bindingfunc *)&psys_rsp_attach;
    b->handlers[PSYS_RSP_IORESULT]     = (psys_bindingfunc *)&psys_rsp_ioresult;
    b->handlers[PSYS_RSP_UNITBUSY]     = (psys_bindingfunc *)&psys_rsp_unitbusy;
    b->handlers[PSYS_RSP_POWEROFTEN]   = (psys_bindingfunc *)&psys_rsp_poweroften;
    b->handlers[PSYS_RSP_UNITWAIT]     = (psys_bindingfunc *)&psys_rsp_unitwait;
    b->handlers[PSYS_RSP_UNITCLEAR]    = (psys_bindingfunc *)&psys_rsp_unitclear;
    b->handlers[PSYS_RSP_UNITSTATUS]   = (psys_bindingfunc *)&psys_rsp_unitstatus;
    b->handlers[PSYS_RSP_IDSEARCH]     = (psys_bindingfunc *)&psys_rsp_idsearch;
    b->handlers[PSYS_RSP_TREESEARCH]   = (psys_bindingfunc *)&psys_rsp_treesearch;
    b->handlers[PSYS_RSP_READSEG]      = (psys_bindingfunc *)&psys_rsp_readseg;
    /* these are simply aliases */
    b->handlers[PSYS_RSP_UNITREAD2]     = (psys_bindingfunc *)&psys_rsp_unitread;
    b->handlers[PSYS_RSP_UNITWRITE2]    = (psys_bindingfunc *)&psys_rsp_unitwrite;
    b->handlers[PSYS_RSP_UNITBUSY2]     = (psys_bindingfunc *)&psys_rsp_unitbusy;
    b->handlers[PSYS_RSP_UNITWAIT2]     = (psys_bindingfunc *)&psys_rsp_unitwait;
    b->handlers[PSYS_RSP_UNITCLEAR2]    = (psys_bindingfunc *)&psys_rsp_unitclear;
    b->handlers[PSYS_RSP_UNITSTATUS2]   = (psys_bindingfunc *)&psys_rsp_unitstatus;
    b->handlers[PSYS_RSP_READSEG2]      = (psys_bindingfunc *)&psys_rsp_readseg;
    b->handlers[PSYS_RSP_SETRESTRICTED] = (psys_bindingfunc *)&psys_rsp_setrestricted;

    b->save_state = &psys_rsp_save_state;
    b->load_state = &psys_rsp_load_state;

    return b;
}

void psys_destroy_rsp(struct psys_binding *b)
{
    struct psys_rsp_state *rsp = (struct psys_rsp_state *)b->userdata;
    free(rsp->disk0);
    free(rsp);
    free(b->handlers);
    free(b);
}

void psys_rsp_set_disk(struct psys_binding *b, int n, void *data, size_t size, size_t track, bool wrap)
{
    struct psys_rsp_state *rsp = (struct psys_rsp_state *)b->userdata;
    if (n != 0)
        psys_panic("RSP only support disk 0 for now\n");
    if ((size % PSYS_BLOCK_SIZE) != 0)
        psys_panic("Disk size must be multiple of block size %d\n", PSYS_BLOCK_SIZE);
    rsp->disk0       = data;
    rsp->disk0_size  = size / PSYS_BLOCK_SIZE;
    rsp->disk0_track = track / PSYS_BLOCK_SIZE;
    rsp->disk0_wrap  = wrap;
}

void psys_rsp_event(struct psys_binding *b, psys_word event, bool taskswitch)
{
    struct psys_rsp_state *rsp = (struct psys_rsp_state *)b->userdata;
    if (event >= PSYS_MAX_EVENTS || rsp->events[event] == 0) {
        return;
    }
    /* Signal semaphore */
    psys_signal(rsp->psys, rsp->events[event], taskswitch);
}

void psys_rsp_settime(struct psys_binding *b, psys_word time)
{
    struct psys_rsp_state *rsp = (struct psys_rsp_state *)b->userdata;
    rsp->time                  = time;
}
