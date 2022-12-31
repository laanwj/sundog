/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "psys_task.h"
#include "psys_constants.h"
#include "psys_debug.h"
#include "psys_helpers.h"
#include "psys_state.h"

/* Take TIB from head of queue */
static psys_word take_queue_head(struct psys_state *s, psys_word *head)
{
    psys_word tib = *head;
    if (tib == PSYS_NIL) {
        return PSYS_NIL;
    } else {
        /* Put next task (if any) at queue head */
        psys_word next = psys_ldw(s, tib + PSYS_TIB_Wait_Q);
        *head          = next;
        return tib;
    }
}

static psys_word get_task_priority(struct psys_state *s, psys_word tib)
{
    return psys_ldw(s, tib + PSYS_TIB_Flags_Prior) & 0xff;
}

static void dump_queue(struct psys_state *s, const char *name, psys_word head)
{
    psys_word ptr = head;
    psys_debug("Dump of %s queue:\n", name);
    while (ptr != PSYS_NIL) {
        psys_word next = psys_ldw(s, ptr + PSYS_TIB_Wait_Q);
        psys_debug("  %04x priority=%3i\n", ptr, get_task_priority(s, ptr));
        ptr = next;
    }
}

/* Put TIB in a queue.
 * Tasks in queues are ordered by decreasing priority.
 */
static void put_queue(struct psys_state *s, psys_word *head, psys_word tib)
{
    psys_word priority = get_task_priority(s, tib);
    psys_word prev     = PSYS_NIL;
    psys_word cur      = *head;
    /* Invariant: tasks must stay ordered by decreasing priority.
     */
    while (cur != PSYS_NIL) {
        if (get_task_priority(s, cur) < priority)
            break;
        prev = cur;
        cur  = psys_ldw(s, cur + PSYS_TIB_Wait_Q);
    }
    if (prev == PSYS_NIL) { /* replace head */
        *head = tib;
    } else { /* replace .next of previous */
        psys_stw(s, prev + PSYS_TIB_Wait_Q, tib);
    }
    /* put old rest of queue after tib */
    psys_stw(s, tib + PSYS_TIB_Wait_Q, cur);
}

/* Switch to task at head of Ready Queue.
 */
void psys_task_switch(struct psys_state *s)
{
    psys_word tib = s->readyq;

    if (PDBG(s, TASK)) {
        psys_debug("task switch curtask=0x%04x readyq=0x%04x\n",
            s->curtask, s->readyq);
    }
    if (s->curtask == tib) { /* Nothing to do */
        return;
    }
    /* store current task state to TIB before switching */
    psys_store_state_to_tib(s);
    s->curtask = tib;
    if (tib == PSYS_NIL) {
        psys_panic("Task switch with no tasks ready to run\n");
    }
    psys_increase_timestamp(s, s->erec);
    if (PDBG(s, TASK)) {
        psys_debug("switching task to tib 0x%04x\n", tib);
    }
    psys_restore_state_from_tib(s);
}

void psys_signal(struct psys_state *s, psys_word semaphore, bool taskswitch)
{
    psys_sword count = psys_ldw(s, semaphore + PSYS_SEM_COUNT);
    psys_word qhead  = psys_ldw(s, semaphore + PSYS_SEM_TIB);
    if (PDBG(s, TASK)) {
        psys_debug("signal %04x (%04x %04x)\n", semaphore, count, qhead);

        dump_queue(s, "ready (before)", s->readyq);
        dump_queue(s, "semaphore (before)", qhead);
    }
    /*
     * If the semaphore's wait queue is empty or the count is negative, the
     * count is incremented by one.  Otherwise, the TIB at the head of the
     * semaphore's wait queue is put on the ready queue, and its Hang ptr is
     * set to NIL.  If the new task has a higher priority than the current
     * task, a task switch occurs.
     */
    if (qhead == PSYS_NIL || count < 0) {
        psys_stw(s, semaphore + PSYS_SEM_COUNT, count + 1);
        if (PDBG(s, TASK)) {
            psys_debug("increased count to %d\n", count + 1);
        }
    } else {
        /* Take first tib from semaphore queue */
        psys_word tib = take_queue_head(s, &qhead);
        /* Clear hang pointer */
        psys_stw(s, tib + PSYS_TIB_Hang_Ptr, PSYS_NIL);
        /* Store new queue head */
        psys_stw(s, semaphore + PSYS_SEM_TIB, qhead);
        /* Put new tib on ready queue */
        put_queue(s, &s->readyq, tib);
        if (taskswitch) {
            /* Task switch if new task has higher priority than current */
            psys_task_switch(s);
        }
    }
    if (PDBG(s, TASK)) {
        dump_queue(s, "ready (after)", s->readyq);
        dump_queue(s, "semaphore (after)", qhead);
    }
}

void psys_wait(struct psys_state *s, psys_word semaphore)
{
    psys_sword count = psys_ldw(s, semaphore + PSYS_SEM_COUNT);
    psys_word qhead  = psys_ldw(s, semaphore + PSYS_SEM_TIB);
    /*
     * If the semaphore's count is greater than zero, the count is decremented
     * by one. Otherwise, the current TIB is put on the semaphore's wait queue,
     * its Hang_Ptr is set to TOS, and a task switch occurs.
     */
    if (PDBG(s, TASK)) {
        psys_debug("wait %04x (%04x %04x)\n", semaphore, count, qhead);
        dump_queue(s, "ready (before)", s->readyq);
        dump_queue(s, "semaphore (before)", qhead);
    }
    if (count > 0) {
        psys_stw(s, semaphore + PSYS_SEM_COUNT, count - 1);
        if (PDBG(s, TASK)) {
            psys_debug("decreased count to %d\n", count - 1);
        }
    } else {
        /* Take current task from ready queue */
        psys_word tib = take_queue_head(s, &s->readyq);
        /* Put current tib on queue */
        put_queue(s, &qhead, tib);
        /* Store new queue head */
        psys_stw(s, semaphore + PSYS_SEM_TIB, qhead);
        /* Set hang pointer to TOS */
        psys_stw(s, tib + PSYS_TIB_Hang_Ptr, semaphore);
        /* Force task switch */
        psys_task_switch(s);
    }
    if (PDBG(s, TASK)) {
        dump_queue(s, "ready (after)", s->readyq);
        dump_queue(s, "semaphore (after)", qhead);
    }
}

void psys_restore_state_from_tib(struct psys_state *s)
{
    psys_word new_erec;
    psys_word ior_procnum;
    s->sp = psys_ldw(s, s->curtask + PSYS_TIB_SP);
    s->mp = psys_ldw(s, s->curtask + PSYS_TIB_MP);
    /* Change curseg if erec changed.
     * Not sure if it should happen in this function, as the erec will have to be
     * checked for residency before we end up here in the first place
     * (failing at this point will result in inconsistent VM state).
     */
    new_erec = psys_ldw(s, s->curtask + PSYS_TIB_ENV);
    if (s->erec != new_erec) {
        psys_fulladdr new_segment;
        new_segment = psys_segment_from_erec(s, new_erec, true);
        if (new_segment == PSYS_ADDR_ERROR) {
            /* TODO: this should probably trigger a fault and correctly go forward instead */
            psys_panic("TIB restore to non-resident segment\n");
        }
        s->erec   = new_erec;
        s->curseg = new_segment;
    }
    s->ipc      = s->curseg + psys_ldw(s, s->curtask + PSYS_TIB_IPC);
    ior_procnum = psys_ldw(s, s->curtask + PSYS_TIB_IOR_Proc_Num);
    psys_stw(s, s->syscom + PSYS_SYSCOM_IORSLT, ior_procnum >> 8);
    s->curproc = ior_procnum & 0xff;
    s->base    = psys_ldw(s, new_erec + PSYS_EREC_Env_Data);
}

void psys_store_state_to_tib(struct psys_state *s)
{
    psys_stw(s, s->curtask + PSYS_TIB_SP, s->sp);
    psys_stw(s, s->curtask + PSYS_TIB_MP, s->mp);
    psys_stw(s, s->curtask + PSYS_TIB_IPC, s->ipc - s->curseg);
    psys_stw(s, s->curtask + PSYS_TIB_ENV, s->erec);
    psys_stw(s, s->curtask + PSYS_TIB_IOR_Proc_Num, (psys_ldw(s, s->syscom + PSYS_SYSCOM_IORSLT) << 8) | s->curproc);
}
