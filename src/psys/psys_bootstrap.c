/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "psys_bootstrap.h"

#include "psys_constants.h"
#include "psys_debug.h"
#include "psys_helpers.h"
#include "psys_rsp.h"
#include "psys_state.h"

#include <string.h>

static const char system_pascal[] = "\x0dSYSTEM.PASCAL";

static void psys_flipw(struct psys_state *s, psys_fulladdr ptr)
{
    psys_stw(s, ptr, psys_flip_endian(psys_ldw(s, ptr)));
}

void psys_bootstrap(struct psys_state *s, const struct psys_bootstrap_info *boot, const psys_byte *disk)
{
    psys_word global_directory_ptr, segment_dict_ptr, userprog_ptr, procdict_ptr;
    psys_word membase_ptr, sib_ptr, evec_ptr, erec_ptr, lmscw_ptr, gmscw_ptr, tib_ptr;
    psys_word syscom_ptr, extm_ptr;
    unsigned ptr, ipc;
    int num_entries, i;
    const unsigned global_directory_ofs = 2 * PSYS_BLOCK_SIZE;
    const psys_word proc_num            = 1;
    unsigned sys_pascal_ofs, userprog_ofs;
    unsigned userprog_codesize, userprog_globals_size;
    psys_word endian;

    s->sp = boot->isp;
    s->sp -= GDIR_SIZE;
    global_directory_ptr = s->sp;
    /* Copy global directory to stack */
    memcpy(psys_bytes(s, global_directory_ptr), &disk[global_directory_ofs], GDIR_SIZE);
    /* TODO: byte-swap global directory if necessary */
    /* endian = psys_ldw(s, global_directory_ptr + 0x02); */
    /* Look for SYSTEM.PASCAL */
    num_entries = psys_ldw(s, global_directory_ptr + GDIR_NUM_ENTRIES);
    psys_debug("psys_bootstrap: %d entries in global directory\n", num_entries);
    ptr = global_directory_ptr + GDIR_ENTRY_SIZE;
    for (i = 0; i < num_entries; ++i) {
        if (!memcmp(psys_bytes(s, ptr + GDIR_ENTRY_NAME), system_pascal, sizeof(system_pascal))) {
            break;
        }
        ptr += GDIR_ENTRY_SIZE;
    }
    if (i == num_entries) {
        psys_panic("SYSTEM.PASCAL not found\n");
    }
    sys_pascal_ofs = psys_ldw(s, ptr + GDIR_ENTRY_BLOCK) * PSYS_BLOCK_SIZE;
    psys_debug("Found SYSTEM.PASCAL at offset %05x\n", sys_pascal_ofs);

    /* Read segment dictionary */
    s->sp -= PSYS_BLOCK_SIZE;
    segment_dict_ptr = s->sp;
    memcpy(psys_bytes(s, segment_dict_ptr), &disk[sys_pascal_ofs], PSYS_BLOCK_SIZE);
    /* TODO: byte-swap segment dictionary if necessary */
    /* endian = psys_ldw(s, segment_dict_ptr + 0x1fe); */
    userprog_ofs          = sys_pascal_ofs + psys_ldw(s, segment_dict_ptr + 0x3c) * PSYS_BLOCK_SIZE;
    userprog_codesize     = psys_ldw(s, segment_dict_ptr + 0x3e) * 2;
    userprog_globals_size = psys_ldw(s, segment_dict_ptr + 0x120) * 2;
    psys_debug("Found USERPROG at offset 0x%05x, size 0x%05x, globals_size 0x%04x\n", userprog_ofs, userprog_codesize, userprog_globals_size);

    /* Read USERPROG */
    s->sp -= userprog_codesize;
    userprog_ptr = s->sp;
    memcpy(psys_bytes(s, userprog_ptr), &disk[userprog_ofs], userprog_codesize);
    /* byte-swap USERPROG if necessary */
    endian = psys_ldw(s, userprog_ptr + PSYS_SEG_ENDIAN);
    if (endian != 1) {
        psys_word num_proc;
        psys_debug("  Needs byteswap\n");
        psys_flipw(s, userprog_ptr + PSYS_SEG_PROCDICT);
        psys_flipw(s, userprog_ptr + PSYS_SEG_RELOCS);
        psys_flipw(s, userprog_ptr + PSYS_SEG_CPOOLOFS);
        psys_flipw(s, userprog_ptr + PSYS_SEG_REALSIZE);
        psys_flipw(s, userprog_ptr + PSYS_SEG_PARTNUM1);
        psys_flipw(s, userprog_ptr + PSYS_SEG_PARTNUM2);
        procdict_ptr = W(userprog_ptr, psys_ldw(s, userprog_ptr + PSYS_SEG_PROCDICT));
        psys_flipw(s, procdict_ptr);
        num_proc = psys_ldw(s, procdict_ptr);
        for (i = 1; i <= num_proc; ++i) {
            psys_fulladdr proc_ptr;
            psys_flipw(s, W(procdict_ptr, -i)); /* flip procedure pointer */
            proc_ptr = W(userprog_ptr, psys_ldw(s, W(procdict_ptr, -i)));
            psys_flipw(s, W(proc_ptr, -1)); /* flip end pointer */
            psys_flipw(s, W(proc_ptr, 0));  /* flip num locals */
        }
    }

    /* Determine initial program counter (relative to segment).
     * At procedure USERPROG:0x01.
     */
    procdict_ptr = W(userprog_ptr, psys_ldw(s, userprog_ptr + PSYS_SEG_PROCDICT));
    ipc          = psys_ldw(s, W(procdict_ptr, -proc_num)) * 2 + 2;
    psys_debug("Initial IPC at 0x%04x:0x%02x:0x%04x\n", userprog_ptr, proc_num, ipc);

    /* Reserve space for some initial data structures from stack */
    s->sp -= PSYS_SIB_SIZE;
    sib_ptr = s->sp;
    s->sp -= PSYS_EREC_SIZE;
    erec_ptr = s->sp;
    s->sp -= 6; /* size + 2 procedures */
    evec_ptr = s->sp;

    s->sp -= PSYS_MSCW_SIZE;
    lmscw_ptr = s->sp;

    /* Reserve space for some initial data structures from base of memory */
    membase_ptr = 0;
    syscom_ptr  = membase_ptr;
    membase_ptr += PSYS_SYSCOM_SIZE;
    extm_ptr = membase_ptr;
    membase_ptr += PSYS_EXTM_SIZE;
    tib_ptr = membase_ptr;
    membase_ptr += PSYS_TIB_SIZE;
    gmscw_ptr = membase_ptr;

    /* Initialize initial TIB */
    psys_debug("  Initial TIB at 0x%04x\n", tib_ptr);
    psys_stw(s, tib_ptr + PSYS_TIB_Wait_Q, 0x0000);
    psys_stw(s, tib_ptr + PSYS_TIB_Flags_Prior, 0x0080);
    psys_stw(s, tib_ptr + PSYS_TIB_SP_Low, gmscw_ptr + PSYS_MSCW_SIZE + userprog_globals_size);
    psys_stw(s, tib_ptr + PSYS_TIB_SP_Upr, boot->isp);
    psys_stw(s, tib_ptr + PSYS_TIB_SP, s->sp);
    psys_stw(s, tib_ptr + PSYS_TIB_MP, lmscw_ptr);
    psys_stw(s, tib_ptr + PSYS_TIB_Reserved, 0x0000);
    psys_stw(s, tib_ptr + PSYS_TIB_IPC, ipc);
    psys_stw(s, tib_ptr + PSYS_TIB_ENV, erec_ptr);
    psys_stw(s, tib_ptr + PSYS_TIB_IOR_Proc_Num, proc_num);
    psys_stw(s, tib_ptr + PSYS_TIB_Hang_Ptr, 0x0000);
    psys_stw(s, tib_ptr + PSYS_TIB_M_Depend, 0x0000);
    psys_stw(s, tib_ptr + PSYS_TIB_Main_Task, 0x0003);
    psys_stw(s, tib_ptr + PSYS_TIB_Start_MSCW, gmscw_ptr);

    /* Initialize initial EVEC */
    psys_debug("  Initial EVEC at 0x%04x\n", evec_ptr);
    psys_stw(s, W(evec_ptr, 0), 2);
    psys_stw(s, W(evec_ptr, 1), 0);
    psys_stw(s, W(evec_ptr, 2), erec_ptr);

    /* Initialize initial EREC */
    psys_debug("  Initial EREC at 0x%04x\n", erec_ptr);
    psys_stw(s, erec_ptr + PSYS_EREC_Env_Data, gmscw_ptr);
    psys_stw(s, erec_ptr + PSYS_EREC_Env_Vect, evec_ptr);
    psys_stw(s, erec_ptr + PSYS_EREC_Env_SIB, sib_ptr);
    psys_stw(s, erec_ptr + PSYS_EREC_Link_Count, 0);
    psys_stw(s, erec_ptr + PSYS_EREC_Next_Rec, 0);

    /* Initialize initial SIB */
    psys_debug("  Initial SIB at 0x%04x\n", sib_ptr);
    psys_stw(s, sib_ptr + PSYS_SIB_Seg_Pool, 0);
    psys_stw(s, sib_ptr + PSYS_SIB_Seg_Base, userprog_ptr);
    psys_stw(s, sib_ptr + PSYS_SIB_Seg_Refs, 1);
    psys_stw(s, sib_ptr + PSYS_SIB_Time_Stamp, 0);
    psys_stw(s, sib_ptr + PSYS_SIB_Link_Count, 0);
    psys_stw(s, sib_ptr + PSYS_SIB_Residency, 0);
    memset(psys_bytes(s, sib_ptr + PSYS_SIB_Seg_Name), 0, 8);
    psys_stw(s, sib_ptr + PSYS_SIB_Seg_Leng, userprog_codesize / 2);
    psys_stw(s, sib_ptr + PSYS_SIB_Seg_Addr, userprog_ofs / PSYS_BLOCK_SIZE);
    psys_stw(s, sib_ptr + PSYS_SIB_Vol_Info, 0);
    psys_stw(s, sib_ptr + PSYS_SIB_Data_Size, 0);
    psys_stw(s, sib_ptr + PSYS_SIB_Next_Sib, 0);
    psys_stw(s, sib_ptr + PSYS_SIB_Prev_Sib, 0);
    psys_stw(s, sib_ptr + PSYS_SIB_Scratch, 0);
    psys_stw(s, sib_ptr + PSYS_SIB_M_Type, 0);

    /* Initialize initial global MSCW */
    psys_debug("  Initial global MSCW at 0x%04x\n", gmscw_ptr);
    psys_stw(s, gmscw_ptr + PSYS_MSCW_MSSTAT, gmscw_ptr);
    psys_stw(s, gmscw_ptr + PSYS_MSCW_MSDYN, gmscw_ptr);
    psys_stw(s, gmscw_ptr + PSYS_MSCW_IPC, ipc);
    psys_stw(s, gmscw_ptr + PSYS_MSCW_MSENV, 0);
    psys_stw(s, gmscw_ptr + PSYS_MSCW_MPROC, proc_num);

    /* Initialize initial local MSCW */
    psys_debug("  Initial local MSCW at 0x%04x\n", lmscw_ptr);
    psys_stw(s, lmscw_ptr + PSYS_MSCW_MSSTAT, gmscw_ptr);
    psys_stw(s, lmscw_ptr + PSYS_MSCW_MSDYN, gmscw_ptr);
    psys_stw(s, lmscw_ptr + PSYS_MSCW_IPC, ipc);
    psys_stw(s, lmscw_ptr + PSYS_MSCW_MSENV, erec_ptr);
    psys_stw(s, lmscw_ptr + PSYS_MSCW_MPROC, proc_num);

    /* Initialize syscom */
    psys_debug("  SYSCOM at 0x%04x\n", syscom_ptr);
    memset(psys_bytes(s, syscom_ptr), 0, PSYS_SYSCOM_SIZE);
    psys_stw(s, syscom_ptr + PSYS_SYSCOM_IORSLT, PSYS_IO_NOERROR);
    psys_stw(s, syscom_ptr + PSYS_SYSCOM_BOOT_UNIT, boot->boot_unit_id);
    psys_stw(s, syscom_ptr + PSYS_SYSCOM_GLOBALDIR, global_directory_ptr);
    psys_stw(s, syscom_ptr + 0x1a, 0x80); /* ? */
    psys_stw(s, syscom_ptr + 0x32, 0x0b); /* ? */
    psys_stw(s, syscom_ptr + PSYS_SYSCOM_EXTM, extm_ptr);
    psys_stw(s, syscom_ptr + 0x36, 0x02); /* ? */
    psys_stw(s, syscom_ptr + PSYS_SYSCOM_REAL_SIZE, boot->real_size);
    psys_stw(s, syscom_ptr + 0x5c, 0x7f); /* Character mask? */

    /* Initialize EXTM */
    psys_debug("  EXTM at 0x%04x\n", extm_ptr);
    psys_stw(s, extm_ptr + PSYS_EXTM_HEAD, 0x0006);
    psys_stw(s, extm_ptr + PSYS_EXTM_BASE, boot->ext_mem_base >> 16);
    psys_stw(s, extm_ptr + PSYS_EXTM_BASE + 2, boot->ext_mem_base & 0xffff);
    psys_stw(s, extm_ptr + PSYS_EXTM_BLOCKS, boot->ext_mem_size);

    /* Set up initial registers */
    s->ipc     = userprog_ptr + ipc;
    s->base    = gmscw_ptr;
    s->mp      = lmscw_ptr;
    s->curseg  = userprog_ptr;
    s->readyq  = tib_ptr;
    s->curtask = tib_ptr;
    s->erec    = erec_ptr;
    s->curproc = proc_num;

    /* Other offsets and state */
    s->syscom        = syscom_ptr;
    s->mem_fake_base = boot->mem_fake_base;
}
