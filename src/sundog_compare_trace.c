/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "psys/psys_bootstrap.h"
#include "psys/psys_constants.h"
#include "psys/psys_debug.h"
#include "psys/psys_helpers.h"
#include "psys/psys_interpreter.h"
#include "psys/psys_opcodes.h"
#include "psys/psys_rsp.h"
#include "psys/psys_task.h"

#include "util/memutil.h"
#include "util/util_minmax.h"

#include "game/game_gembind.h"
#include "game/game_screen.h"
#include "game/game_shiplib.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int tracefd;
uint32_t tracerecsize;
int traceskip; /* number of steps to skip */
int tracecount;

#define TRACE_STACK_BYTES 128
#define TRACE_SYSCOM_BYTES 128
struct __attribute__((__packed__)) psys_tracerec {
    uint32_t curseg;
    uint32_t ipc;
    uint16_t sp;
    uint16_t mp;
    uint16_t base;
    uint16_t readyq;
    uint16_t evec;
    uint16_t curtask;
    uint16_t erec;
    uint16_t curproc;
    uint16_t event; /* number of events triggered before this instruction */
    uint8_t stack[TRACE_STACK_BYTES];
    uint8_t syscom[TRACE_SYSCOM_BYTES];
    uint32_t time;
};

static void setup_trace_compare()
{
    tracefd = open("../sundog.psystrace", O_RDONLY);
    if (tracefd < 0) {
        perror("Could not open trace file\n");
        exit(1);
    }
    if (read(tracefd, &tracerecsize, 4) < 0) {
        perror("Trace file read error\n");
        exit(1);
    }
    if (tracerecsize != sizeof(struct psys_tracerec)) {
        fprintf(stderr, "Trace record size mismatch (%d versus %d)\n", (int)tracerecsize, (int)sizeof(struct psys_tracerec));
        exit(1);
    }
    traceskip  = 2;
    tracecount = 0;
}

static void detect_mismatch(struct psys_state *s, struct psys_tracerec *rec)
{
    int x;
    int mismatch = false;
    int mpofs    = umin((s->mp - s->sp) / 2 + 5, TRACE_STACK_BYTES / 2);
    /* int mpofs = TRACE_STACK_BYTES / 2; */
    /* only compare current stack frame and upmost MSCW for now, as locals have
     * uninitialized crap in them,
      this would need a patch to the interpreter to get a sane trace */
    for (x = 0; x < (mpofs * 2); ++x) {
        if (psys_ldb(s, s->sp, x) != rec->stack[x]) {
            printf("mismatch at stack offset %02x\n", x);
            mismatch = true;
        }
    }
    for (x = 0x0; x < 0x60; ++x) { /* compare only part of syscom for now */
        if (psys_ldb(s, 0, x) != rec->syscom[x]) {
            printf("mismatch at syscom offset 0x%02x (0x%02x versus 0x%02x)\n", x, psys_ldb(s, 0, x), rec->syscom[x]);
            mismatch = true;
        }
    }
    if (mismatch
        || s->ipc != rec->ipc
        || s->sp != rec->sp
        || s->base != rec->base
        || s->mp != rec->mp
        || s->curseg != rec->curseg
        || s->readyq != rec->readyq
        || s->curtask != rec->curtask
        || s->erec != rec->erec
        || s->curproc != rec->curproc) {
        printf("\x1b[41;30m Mismatch detected between trace and run! \x1b[0m\n");
        printf("(after %i steps - our versus ref)\n", tracecount);
        printf("ipc    %05x:%04x %05x:%04x\n",
            s->curseg, s->ipc - s->curseg,
            rec->curseg, rec->ipc - rec->curseg);
        printf("sp      %04x  %04x\n", s->sp, rec->sp);
        printf("base    %04x  %04x\n", s->base, rec->base);
        printf("mp      %04x  %04x\n", s->mp, rec->mp);
        printf("readyq  %04x  %04x\n", s->readyq, rec->readyq);
        printf("curtask %04x  %04x\n", s->curtask, rec->curtask);
        printf("erec    %04x  %04x\n", s->erec, rec->erec);
        printf("curproc %04x  %04x\n", s->curproc, rec->curproc);
        printf("stk(our) ");
        for (x = 0; x < mpofs; ++x) {
            printf("%04x ", psys_ldw(s, W(s->sp, x)));
        }
        printf("\n");
        printf("stk(ref) ");
        for (x = 0; x < mpofs; ++x) {
            printf("%04x ", F(((psys_word *)rec->stack)[x]));
        }
        printf("\n");
        printf("com(our)\n");
        psys_debug_hexdump(s, s->syscom, 0x60);
        printf("com(ref)\n");
        psys_debug_hexdump_ofs(rec->syscom, s->syscom, 0x60);

        exit(1);
    }
    tracecount += 1;
}

static psys_byte prev_opc;
static bool taskswitch_later;

/* Called before every instruction executed.
 * Put debug hooks and tracing here.
 * Enable with: state->trace = psys_trace;
 */
static void psys_trace(struct psys_state *s, void *dummy)
{
    struct psys_tracerec rec;
    if (PDBG(s, TRACE)) {
        psys_print_info(s);
    }
    if (traceskip == 0) {
        /* psys_debug("ipc=0x%05x (seg+0x%05x) sp=%04x\n", s->ipc, s->ipc - s->curseg, s->sp); */
        if (read(tracefd, &rec, sizeof(rec)) < (ssize_t)sizeof(rec)) {
            perror("Trace file read error");
            exit(1);
        }
        /* previous opcode was a call? If so fill in initial locals from trace.
         * This makes it possible to match exactly even though the ST version leaves "random"
         * the junk on the stack because the same stack is used for normal calls.
         */
        if (s->local_init_count) {
            unsigned local_init_base = s->local_init_base * 2;
            unsigned num_locals      = umin(s->local_init_count, (TRACE_STACK_BYTES - local_init_base) / 2);
#if 0
            psys_debug("Last opcode was call - filling in %d local initial values\n", num_locals);
#endif
            memcpy(psys_words(s, s->sp + local_init_base), &rec.stack[local_init_base], num_locals * 2);
        }
        /* trigger delayed task switches */
        if (taskswitch_later) {
            psys_task_switch(s);
            taskswitch_later = false;
        }
        /* trigger events before instruction executes */
        if (rec.event) {
            unsigned x;
            bool taskswitch = true;
            if (rec.event & 0x8000) {
                psys_debug("\x1b[30;104mDELAYED EVENT\x1b[0m %d\n", rec.event & 0x7fff);
                taskswitch       = false; /* don't immediately task switch */
                taskswitch_later = true;  /* but before next instruction */
            } else {
                psys_debug("\x1b[30;104mEVENT\x1b[0m %d\n", rec.event & 0x7fff);
            }
            for (x = 0; x < (rec.event & 0x7fff); ++x)
                psys_rsp_event(s->bindings[0], 0, taskswitch);
        }
        /* set system input before instruction executes */
        psys_rsp_settime(s->bindings[0], rec.time);
        /* detect mismatches (do this last) */
        detect_mismatch(s, &rec);
    } else {
        traceskip--;
    }
    prev_opc            = psys_ldb(s, 0, s->ipc);
    s->local_init_count = 0;
}

/*
  D0 00001AB8   D1 0000D5C0   D2 00030000   D3 00030000
  D4 00500000   D5 0200FFFF   D6 00000000   D7 00000000
  A0 00030D6C   A1 00023836   A2 00030DAA   A3 000209AC
  A4 00032862   A5 00020A8A   A6 000237AC   A7 00030D6C
USP  00030D6C ISP  00003DE8
T=00 S=0 M=0 X=0 N=0 Z=0 V=0 C=0 IMASK=3 STP=0
00023036 4ed5                     JMP (A5)
Next PC: 00023038

readyq  0x006e
evec    0xd5ca
curtask 0x006e
erec    0xd5d0
curproc 0x0001
ipc     0xf0b6 = a4-a6 (0x1ab8 = a4-a2 relative to segment)
base    0x008a = a1-a6
mp      0xd5c0 = a0-a6
sp      0xd5c0 = a7-a6
curseg  0xd5fe = a2-a6
*/

static struct psys_state *setup_state(struct game_screen *screen)
{
    struct psys_state *state = CALLOC_STRUCT(psys_state);
    struct psys_bootstrap_info boot;
    int fd;
    psys_byte *disk_data;
    size_t disk_size, track_size;
    psys_word ext_memsize     = 786; /* Memory size in kB */
    psys_fulladdr ext_membase = 0x000337ac;
    psys_byte *verify_mem;
    int errors = 0;
    unsigned ptr;
    struct psys_binding *rspb;

    /* allocate memory */
    state->mem_size = (ext_memsize + 64) * 1024;
    state->memory   = malloc(state->mem_size);
    memset(state->memory, 0, state->mem_size);

    /* load disk image */
    track_size = 9 * 512;
    disk_size  = 80 * track_size;
    disk_data  = malloc(disk_size);
    fd         = open("../verify_disk.st", O_RDONLY);
    if (fd == -1) {
        perror("Could not open disk image\n");
        exit(1);
    }
    if (read(fd, disk_data + 77 * track_size, 3 * track_size) < (ssize_t)(3 * track_size) || read(fd, disk_data, 77 * track_size) < (ssize_t)(77 * track_size)) {
        perror("Could not read disk image\n");
        exit(1);
    }
    close(fd);

    /* override memory size and offset in SYSTEM.MISCINFO */
    /* This is sneaky: at boot, SUNDOG writes amount of memory and memory
     * offset to SYSTEM.MISCINFO, which is read later by the p-machine.
     * Emulate this.
     */
    *((psys_word *)&disk_data[0x1e00 + 0x22]) = F(ext_memsize);
    *((psys_word *)&disk_data[0x1e00 + 0x24]) = F(ext_membase >> 16);
    *((psys_word *)&disk_data[0x1e00 + 0x26]) = F(ext_membase & 0xffff);

    /* Bootstrap */
    boot.boot_unit_id  = PSYS_UNIT_DISK0;
    boot.isp           = 0xfdec;
    boot.real_size     = 0;
    boot.mem_fake_base = 0x000237ac;
    boot.ext_mem_base  = boot.mem_fake_base + boot.isp;
    boot.ext_mem_size  = 0;
    psys_bootstrap(state, &boot, &disk_data[track_size]);

    /* Read initial memory state to compare to */
    verify_mem = malloc(65536);
    fd         = open("../files/psys_initmem.bin", O_RDONLY);
    if (fd == -1) {
        perror("Could not open initial state file\n");
        exit(1);
    }
    if (read(fd, verify_mem, 65536) < 65536) {
        perror("Could not read initial state file\n");
        exit(1);
    }
    close(fd);
    /* Compare initial memory block by block. Stop checking at 0xfde0 (0xfdec)
     * as there is only a sector buffer and a structure only used by the PME
     * (logical to physical sector mapping) there.
     */
    for (ptr = 0; ptr < 0xfde0; ptr += 0x10) {
        if (memcmp(&verify_mem[ptr], &state->memory[ptr], 0x10)) {
            printf("Discrepancy at address 0x%04x:\n", ptr);
            printf("(our) ");
            psys_debug_hexdump_ofs(&state->memory[ptr], ptr, 0x10);
            printf("(ref) ");
            psys_debug_hexdump_ofs(&verify_mem[ptr], ptr, 0x10);
            errors += 1;
        }
    }
    if (errors) {
        printf("%i errors in bootstrapping, exiting\n", errors);
        exit(1);
    }

    /* Assert initial registers */
    assert(state->readyq == 0x006e);
    /* state->evec    = 0xd5ca; */
    assert(state->curtask == 0x006e);
    assert(state->erec == 0xd5d0);
    assert(state->curproc == 0x0001);
    assert(state->ipc == 0xf0b6);
    assert(state->base == 0x008a);
    assert(state->mp == 0xd5c0);
    assert(state->sp == 0xd5c0);
    assert(state->curseg == 0xd5fe);
    assert(state->mem_fake_base == 0x000237ac);

    state->debug = PSYS_DBG_WARNING;
    {
        const char *x = getenv("PSYS_DEBUG");
        if (x) {
            state->debug = strtol(x, NULL, 0);
        }
    }

    /* set up RSP and disk */
    rspb = psys_new_rsp(state);

    psys_rsp_set_disk(rspb, 0, disk_data, disk_size, track_size, true);

    /* Set up bindings */
    state->num_bindings = 3;
    state->bindings     = calloc(state->num_bindings, sizeof(struct binding *));
    state->bindings[0]  = rspb;
    state->bindings[1]  = new_shiplib(state, screen);
    state->bindings[2]  = new_gembind(state, screen, 0);

    /* Debugging */
    state->trace = psys_trace;

    return state;
}

int main(int argc, char **argv)
{
    struct psys_state *state;
    struct game_screen *screen = NULL;

    screen = new_game_screen();

    state = setup_state(screen);
    setup_trace_compare();
    psys_interpreter(state);

    screen->destroy(screen);
    return 0;
}
