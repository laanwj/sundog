/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "debugger.h"

#include "psys/psys_constants.h"
#include "psys/psys_debug.h"
#include "psys/psys_helpers.h"
#include "psys/psys_registers.h"
#include "psys/psys_state.h"
#include "util/memutil.h"

#include "compat/compat_fcntl.h"
#include "compat/compat_unistd.h"
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <string.h>

/** Rudimentary p-system debugger. The following functionality would be nice:
 * - Load / set VM register (lpr/spr)
 * - Breakpoint on data change
 * - Break on runtime fault / error
 * - Show static closure (static links above current stackframe)
 * - Access intermediate variables (lda, lod, str) (can be done through ups/downs and locals manipulation)
 * - Call procedure (with arguments) (create fake MSCW)
 * - Expression evaluation (convenience)
 * - Switch task
 * - Change task priority
 * - List tasks (nontrivial, unlike for erecs there seems to be no linked directory of tasks)
 * - Create/compare execution trace
 * - Make resident/swap out segment (can be done as soon as calling procedures is possible)
 * - Throw runtime fault / error
 * - Print heap statistics
 * - Disassemble range
 * - Set/clear interpreter debug flags
 */

#define MAX_BREAKPOINTS 40

#define ATITLE "\x1b[38;5;202;48;5;235m"
#define ASUBTITLE "\x1b[38;5;214m"
#define ARESET "\x1b[0m"

/** One debugger breakpoint */
struct dbg_breakpoint {
    bool used;
    bool active;
    int num;
    /* Only one type of breakpoint for now:
     * by segment:offset code address.
     */
    struct psys_segment_id seg; /* Segment name */
    psys_word addr;             /* Address relative to segment */
};

/** Description of one stack frame, with forward
 * and backward links.
 */
struct dbg_stackframe {
    struct psys_segment_id seg; /* Segment name */
    psys_word curproc;
    psys_word mp;
    psys_word base;
    psys_word erec;
    psys_word ipc; /* Address relative to segment */
    psys_word sib;
    psys_word msstat;
    /* Double-linked list. Stacks grow downward so "up" refers to the caller
     * and "down" to the callee.
     */
    struct dbg_stackframe *down, *up;
};

/** Global debugger mode */
enum debugger_mode {
    DM_NONE,
    DM_SINGLE_STEP, /* Single-stepping */
    DM_STEP_OUT,    /* Stepping out of function */
    DM_STEP_OVER,   /* Stepping over (next instruction in function or parent) */
};

/** Global debugger state */
struct psys_debugger {
    struct psys_state *state;
    /* While single-stepping this prevents stopping immediately on the same
     * instruction after the interpreter thread starts.
     */
    psys_word curerec;
    psys_word curipc;
    /* Number of breakpoint entries used */
    int num_breakpoints;
    /* Breakpoints */
    struct dbg_breakpoint breakpoints[MAX_BREAKPOINTS];
    /* Current debugger mode */
    enum debugger_mode mode;
    /* Currently selected task (used to filter single-stepping) */
    psys_word curtask;
    /* Currently selected frame pointers (used for step-out and step-over) */
    psys_word target_mp1;
    psys_word target_mp2;
};

/** Primitive argument parsing / splitting.
 * Separates out arguments separated by any number of spaces.
 */
static unsigned parse_args(char **args, unsigned max, char *line)
{
    char *arg    = line;
    unsigned num = 0;
    if (!line)
        return 0;
    /* skip initial spaces */
    while (*arg == ' ') {
        ++arg;
    }
    /* split arguments */
    while (arg && *arg && num < max) {
        char *nextarg = strchr(arg, ' ');
        args[num++]   = arg;
        if (nextarg) { /* if separator found, move past spaces to argument */
            *nextarg = '\0';
            ++nextarg;
            while (*nextarg == ' ') {
                ++nextarg;
            }
        }
        arg = nextarg;
    }
    return num;
}

/** Get segment residency status for an erec */
static bool is_segment_resident(struct psys_state *s, psys_word erec)
{
    psys_word sib = psys_ldw(s, erec + PSYS_EREC_Env_SIB);
    /* seg_base will be 0 if not resident */
    psys_word seg_base = psys_ldw(s, sib + PSYS_SIB_Seg_Base);
    return seg_base != 0 ? true : false;
}

/** Get a free breakpoint */
static struct dbg_breakpoint *new_breakpoint(struct psys_debugger *dbg)
{
    int i;
    for (i = 0; i < MAX_BREAKPOINTS; ++i) {
        struct dbg_breakpoint *brk = &dbg->breakpoints[i];
        if (!brk->used) {
            /* Adjust num_breakpoints to include the newly allocated slot */
            if (i >= dbg->num_breakpoints) {
                dbg->num_breakpoints = i + 1;
            }
            brk->used = true;
            brk->num  = i;
            return brk;
        }
    }
    return NULL;
}

/*** Frame handling ***/
static void stack_frame_print(struct psys_state *s, struct dbg_stackframe *frame)
{
    psys_debug("  %-8.8s:0x%02x:%04x mp=0x%04x base=0x%04x erec=0x%04x\n",
        frame->seg.name, frame->curproc, frame->ipc,
        frame->mp, frame->base, frame->erec);
}

/** Create doubly-linked list from stack frame.
 * Returns downmost frame. The result must be freed with
 * stack_frames_free.
 */
static struct dbg_stackframe *get_stack_frames(struct psys_state *s)
{
    psys_word mp_up;
    struct dbg_stackframe *frame, *lowest;
    lowest = frame = CALLOC_STRUCT(dbg_stackframe);
    frame->curproc = s->curproc;
    frame->ipc     = s->ipc - s->curseg;
    frame->mp      = s->mp;
    frame->base    = s->base;
    frame->erec    = s->erec;

    while (true) {
        struct dbg_stackframe *frame_down;
        /** Fill in sib, segment name and static link for current frame */
        frame->sib = psys_ldw(s, frame->erec + PSYS_EREC_Env_SIB);
        memcpy(&frame->seg, psys_bytes(s, frame->sib + PSYS_SIB_Seg_Name), 8);
        frame->msstat = psys_ldw(s, frame->mp + PSYS_MSCW_MSSTAT);

        /* Advance to caller frame, if there are any */
        mp_up = psys_ldw(s, frame->mp + PSYS_MSCW_MSDYN);
        if (mp_up == 0 || frame->mp == mp_up) {
            break; /* At the top, nothing more to do */
        }
        /* Create and attach frame */
        frame_down     = frame;
        frame          = CALLOC_STRUCT(dbg_stackframe);
        frame->down    = frame_down;
        frame_down->up = frame;

        /* As these are stored at procedure call time, take them from caller
         * frame */
        frame->mp      = mp_up;
        frame->erec    = psys_ldw(s, frame_down->mp + PSYS_MSCW_MSENV);
        frame->curproc = psys_ldsw(s, frame_down->mp + PSYS_MSCW_MPROC);
        frame->ipc     = psys_ldw(s, frame_down->mp + PSYS_MSCW_IPC);
        /* Get new base pointer from erec */
        frame->base = psys_ldw(s, frame->erec + PSYS_EREC_Env_Data);
    }
    return lowest;
}

/** Free stack frames structure (starting at downmost frame) */
static void stack_frames_free(struct dbg_stackframe *frame)
{
    struct dbg_stackframe *next;
    while (frame) {
        next = frame->up;
        free(frame);
        frame = next;
    }
}

/** Get static/lexical parent of stack frame, or NULL */
static struct dbg_stackframe *stack_frame_static_parent(struct dbg_stackframe *frame)
{
    psys_word msstat = frame->msstat;
    while (frame && frame->mp != msstat) {
        frame = frame->up;
    }
    return frame;
}

/** Get static/lexical child of stack frame, or NULL.
 * If there are more activation records that refer to lexical children of this procedure,
 * this returns the first one encountered.
 */
static struct dbg_stackframe *stack_frame_static_child(struct dbg_stackframe *frame)
{
    psys_word mp = frame->mp;
    while (frame && frame->msstat != mp) {
        frame = frame->down;
    }
    return frame;
}

/** Maximum number of arguments for any debugger command.
 */
#define MAX_ARGS (10)

void psys_debugger_run(struct psys_debugger *dbg, bool user)
{
    struct psys_state *s = dbg->state;
    struct dbg_stackframe *frames, *frame;
    char *line = NULL;
    if (!user) {
        psys_print_info(s);
        dbg->mode = DM_NONE;
    } else {
        printf("** Entering psys debugger: type 'h' for help **\n");
    }

    frame = frames = get_stack_frames(s);
    while (true) {
        bool isrepeat; /* Repeat flag: could be used to continue memory dump/disassembly */
        char *args[MAX_ARGS];
        char *cmd;
        unsigned num;

        line = readline("\x1b[38;5;220mpsys\x1b[38;5;235m>\x1b[38;5;238m>\x1b[38;5;242m>\x1b[0m ");

        if (!line) {
            /* EOF / Ctrl-D */
            exit(1);
        } else if (!*line) {
            /* Empty line: repeat last command */
            HIST_ENTRY *h = previous_history();
            if (h) {
                free(line);
                line     = strdup(h->line);
                isrepeat = true;
            }
        } else {
            add_history(line);
            isrepeat = false;
        }
        /* num is number of tokens, including command */
        num = parse_args(args, MAX_ARGS, line);
        (void)isrepeat; /* unused for now */

        if (num) {
            cmd = args[0];

            if (!strcmp(cmd, "q")) {
                exit(1);
            } else if (!strcmp(cmd, "b")) { /* Set breakpoint or list breakpoints */
                if (num >= 3) {
                    struct dbg_breakpoint *brk = new_breakpoint(dbg);
                    psys_debug_assign_segment_id(&brk->seg, args[1]);
                    brk->addr   = strtol(args[2], NULL, 0);
                    brk->active = true;
                    printf("Set breakpoint %d at %.8s:0x%x\n", brk->num, brk->seg.name, brk->addr);
                } else {
                    int i;
                    printf(ATITLE ">   Breakpoints   >" ARESET "\n");
                    for (i = 0; i < dbg->num_breakpoints; ++i) {
                        struct dbg_breakpoint *brk = &dbg->breakpoints[i];
                        if (brk->used) {
                            printf("%2d %d at %.8s:0x%x\n", i, brk->active, brk->seg.name, brk->addr);
                        }
                    }
                }
            } else if (!strcmp(cmd, "db") || !strcmp(cmd, "eb")) { /* Disable or enable breakpoint */
                if (num >= 2) {
                    int i       = strtol(args[1], NULL, 0);
                    bool enable = !strcmp(cmd, "eb");
                    if (i >= 0 && i < dbg->num_breakpoints) {
                        struct dbg_breakpoint *brk = &dbg->breakpoints[i];
                        brk->active                = enable;
                        printf("%s breakpoint %d\n", enable ? "Enabled" : "Disabled", brk->num);
                    } else {
                        printf("Breakpoint %i out of range\n", i);
                    }
                } else {
                    printf("One argument required\n");
                }
            } else if (!strcmp(cmd, "l")) { /* List segments */
                psys_word erec = psys_debug_first_erec_ptr(s);
                printf(ATITLE "erec sib  flg segname  base size " ARESET "\n");
                while (erec) {
                    psys_word sib       = psys_ldw(s, erec + PSYS_EREC_Env_SIB);
                    psys_word data_base = psys_ldw(s, erec + PSYS_EREC_Env_Data);    /* globals start */
                    psys_word data_size = psys_ldw(s, sib + PSYS_SIB_Data_Size) * 2; /* globals size in bytes */
                    psys_word evec      = psys_ldw(s, erec + PSYS_EREC_Env_Vect);
                    psys_word num_evec  = psys_ldw(s, W(evec, 0));

                    unsigned i;
                    /* Print main segment */
                    printf("%04x   %04x %c %-8.8s %04x %04x\n",
                        erec, sib,
                        is_segment_resident(s, erec) ? 'R' : '-',
                        psys_bytes(s, sib + PSYS_SIB_Seg_Name),
                        data_base, data_size);
                    /* Subsidiary segments. These will be referenced in the segment's evec
                     * and have the same evec pointer (and the same BASE, but that's less reliable
                     * as some segments have no globals).
                     */
                    for (i = 1; i < num_evec; ++i) {
                        psys_word serec = psys_ldw(s, W(evec, i));
                        if (serec) {
                            psys_word ssib  = psys_ldw(s, serec + PSYS_EREC_Env_SIB);
                            psys_word sevec = psys_ldw(s, serec + PSYS_EREC_Env_Vect);
                            if (serec != erec && sevec == evec) {
                                printf("  %04x %04x %c %-8.8s\n",
                                    serec, ssib,
                                    is_segment_resident(s, serec) ? 'R' : '-',
                                    psys_bytes(s, ssib + PSYS_SIB_Seg_Name));
                            }
                        }
                    }
                    erec = psys_ldw(s, erec + PSYS_EREC_Next_Rec);
                }
            } else if (!strcmp(cmd, "bt")) { /* Print backtrace */
                psys_print_traceback(s);
            } else if (!strcmp(cmd, "s") || !strcmp(cmd, "c") || !strcmp(cmd, "so") || !strcmp(cmd, "n")) { /* Single-step or continue or step out */
                dbg->curtask = s->curtask;
                dbg->curerec = s->erec;
                dbg->curipc  = s->ipc - s->curseg;
                if (!strcmp(cmd, "s")) {
                    dbg->mode = DM_SINGLE_STEP;
                } else if (!strcmp(cmd, "so") && frame->up) {
                    dbg->mode       = DM_STEP_OUT;
                    dbg->target_mp1 = frame->up->mp;
                } else if (!strcmp(cmd, "n") && frames->up) {
                    dbg->mode       = DM_STEP_OVER;
                    dbg->target_mp1 = frames->mp;
                    dbg->target_mp2 = frames->up->mp;
                }
                goto cleanup;
            } else if (!strcmp(cmd, "dm")) { /* Dump memory */
                if (num >= 2) {
                    int fd = open(args[1], O_CREAT | O_WRONLY | O_BINARY, 0666);
                    int rv = write(fd, s->memory, s->mem_size);
                    (void)rv;
                    close(fd);
                    printf("Wrote memory dump to %s\n", args[1]);
                } else {
                    printf("Two arguments required\n");
                }
            } else if (!strcmp(cmd, "x")) { /* Examine memory */
                if (num >= 2) {
                    unsigned size = 0x100;
                    if (num >= 3) {
                        size = strtol(args[2], NULL, 0);
                    }
                    psys_debug_hexdump(s, strtol(args[1], NULL, 0), size);
                } else {
                    printf("One or two arguments required\n");
                }
            } else if (!strcmp(cmd, "ww")) { /* write word */
                if (num >= 3) {
                    unsigned addr = strtol(args[1], NULL, 0);
                    unsigned i;
                    for (i = 2; i < num; ++i) {
                        psys_stw(s, addr + (i - 2) * 2, strtol(args[i], NULL, 0));
                    }
                } else {
                    printf("At least two arguments required\n");
                }
            } else if (!strcmp(cmd, "wb")) { /* write byte */
                if (num >= 3) {
                    unsigned addr = strtol(args[1], NULL, 0);
                    unsigned i;
                    for (i = 2; i < num; ++i) {
                        psys_stb(s, addr, i - 2, strtol(args[i], NULL, 0));
                    }
                } else {
                    printf("At least two arguments required\n");
                }
            } else if (!strcmp(cmd, "sro")) { /* Store global */
                if (num >= 4) {
                    psys_word data_size;
                    psys_fulladdr data_base = psys_debug_get_globals(s, args[1], &data_size);
                    unsigned addr           = strtol(args[2], NULL, 0);
                    if (addr < data_size) {
                        psys_stw(s, W(data_base + PSYS_MSCW_VAROFS, addr),
                            strtol(args[3], NULL, 0));
                    } else {
                        if (data_base == PSYS_NIL) {
                            printf("No such segment known\n");
                        } else {
                            printf("Global 0x%x out of range for segment\n", addr);
                        }
                    }
                } else {
                    printf("At least three arguments required\n");
                }
            } else if (!strcmp(cmd, "ldo") || !strcmp(cmd, "lao")) { /* Load global */
                if (num == 3) {
                    psys_word data_size;
                    psys_fulladdr data_base = psys_debug_get_globals(s, args[1], &data_size);
                    unsigned addr           = strtol(args[2], NULL, 0);
                    if (addr < data_size) {
                        if (!strcmp(cmd, "lao")) {
                            psys_word address = W(data_base + PSYS_MSCW_VAROFS, addr);
                            printf("0x%04x\n", address);
                        } else {
                            psys_word value = psys_ldw(s, W(data_base + PSYS_MSCW_VAROFS, addr));
                            printf("0x%04x\n", value);
                        }
                    } else {
                        if (data_base == PSYS_NIL) {
                            printf("No such segment known\n");
                        } else {
                            printf("Global 0x%x out of range for segment\n", addr);
                        }
                    }
                } else {
                    printf("Two arguments are required\n");
                }
            } else if (!strcmp(cmd, "stl")) { /* Store local */
                if (num >= 3) {
                    unsigned addr = strtol(args[1], NULL, 0);
                    psys_stw(s, W(frame->mp + PSYS_MSCW_VAROFS, addr),
                        strtol(args[2], NULL, 0));
                } else {
                    printf("At least two arguments required\n");
                }
            } else if (!strcmp(cmd, "ldl") || !strcmp(cmd, "lla")) { /* Load local */
                if (num == 2) {
                    unsigned addr = strtol(args[1], NULL, 0);
                    if (!strcmp(cmd, "lao")) {
                        psys_word address = W(frame->mp + PSYS_MSCW_VAROFS, addr);
                        printf("0x%04x\n", address);
                    } else {
                        psys_word value = psys_ldw(s, W(frame->mp + PSYS_MSCW_VAROFS, addr));
                        printf("0x%04x\n", value);
                    }
                } else {
                    printf("One argument is required\n");
                }
            } else if (!strcmp(cmd, "r")) { /* Print registers */
                /* TODO: print all registers */
                psys_print_info(s);
            } else if (!strcmp(cmd, "up")) { /* Go to caller stack frame */
                if (frame->up) {
                    frame = frame->up;
                    stack_frame_print(s, frame);
                } else {
                    printf("Cannot go further up\n");
                }
            } else if (!strcmp(cmd, "ups")) { /* Go up static link (lexical parent) */
                struct dbg_stackframe *up = stack_frame_static_parent(frame);
                if (up) {
                    frame = up;
                    stack_frame_print(s, frame);
                } else {
                    printf("No further lexical parent\n");
                }
            } else if (!strcmp(cmd, "down")) { /* Go to callee stack frame */
                if (frame->down) {
                    frame = frame->down;
                    stack_frame_print(s, frame);
                } else {
                    printf("Cannot go further down\n");
                }
            } else if (!strcmp(cmd, "downs")) { /* Go to lexical child */
                struct dbg_stackframe *down = stack_frame_static_child(frame);
                if (down) {
                    frame = down;
                    stack_frame_print(s, frame);
                } else {
                    printf("No lexical children of current frame\n");
                }
            } else if (!strcmp(cmd, "spr")) { /* Store procedure register */
                if (num == 3) {
                    int reg         = strtol(args[1], NULL, 0);
                    psys_word value = strtol(args[2], NULL, 0);
                    psys_spr(s, reg, value);
                } else {
                    printf("Two arguments are required\n");
                }
            } else if (!strcmp(cmd, "lpr")) { /* Load procedure register */
                if (num == 2) {
                    int reg         = strtol(args[1], NULL, 0);
                    psys_word value = psys_lpr(s, reg);
                    printf("0x%04x\n", value);
                } else {
                    printf("One argument is required\n");
                }
            } else if (!strcmp(cmd, "h") || !strcmp(cmd, "?")) {
                printf(ATITLE ">   Help   >" ARESET "\n");
                printf(ASUBTITLE ":Breakpoints:" ARESET "\n");
                printf("b                   List breakpoints\n");
                printf("b <seg> <addr>      Set breakpoint at seg:addr\n");
                printf("db <i>              Disable breakpoint i\n");
                printf("eb <i>              Enable breakpoint i\n");
                printf("bt                  Show backtrace\n");
                printf(ASUBTITLE ":Execution:" ARESET "\n");
                printf("s                   Single-step\n");
                printf("so                  Step out of currently selected stack frame\n");
                printf("n                   Step to next function in current or parent stack frame\n");
                printf("c                   Continue execution\n");
                printf(ASUBTITLE ":Variables:" ARESET "\n");
                printf("lao <seg> <g>       Show address of global variable\n");
                printf("sro <seg> <g> <val> Set global variable\n");
                printf("ldo <seg> <g>       Examine global variable\n");
                printf("lla <l>             Show address of local variable\n");
                printf("stl <l> <val>       Set local variable\n");
                printf("ldl <l>             Examine local variable\n");
                printf(ASUBTITLE ":Stack frames:" ARESET "\n");
                printf("up                  Up to caller frame\n");
                printf("ups                 Up to lexical parent\n");
                printf("down                Down to callee frame\n");
                printf("downs               Down to lexical child\n");
                printf(ASUBTITLE ":Memory:" ARESET "\n");
                printf("dm <file>           Write entire p-system memory to file\n");
                printf("wb <addr> <val> ... Write byte(s) to address\n");
                printf("ww <addr> <val> ... Write word(s) to address\n");
                printf("x <addr>            Examine memory\n");
                printf(ASUBTITLE ":Misc:" ARESET "\n");
                printf("h                   This information\n");
                printf("l                   List environments\n");
                printf("r                   Show registers and current instruction\n");
                printf("lpr <r>             Show register r\n");
                printf("spr <r> <val>       Set register r to val\n");
                printf("q                   Quit\n");
                printf("\n");
                printf("All values can be specified as decimal or 0xHEX.\n");
            } else {
                printf("Unknown command %s. Type 'h' for help.\n", cmd);
            }
        }

        free(line);
        line = NULL;
    }
cleanup:
    /* Clean up state and exit */
    stack_frames_free(frames);
    free(line);
}

/** Called for every instruction - this should break on breakpoints */
bool psys_debugger_trace(struct psys_debugger *dbg)
{
    struct psys_state *s     = dbg->state;
    psys_word sib            = psys_ldw(s, s->erec + PSYS_EREC_Env_SIB);
    const psys_byte *segname = psys_bytes(s, sib + PSYS_SIB_Seg_Name);
    psys_word pc             = s->ipc - s->curseg; /* PC relative to current segment */
    int i;
    if (!(s->erec == dbg->curerec && pc == dbg->curipc)) {
        /* Make sure at least one instruction has been executed after the command was set.
         */
        if (s->curtask == dbg->curtask) {
            /* Make sure at least one instruction has been executed and that
             * we're executing in the right task.
             */
            if (dbg->mode == DM_SINGLE_STEP) {
                return true;
            }
            if (dbg->mode == DM_STEP_OUT && s->mp == dbg->target_mp1) {
                return true;
            }
            if (dbg->mode == DM_STEP_OVER
                && (s->mp == dbg->target_mp1 || s->mp == dbg->target_mp2)) {
                return true;
            }
        }

        for (i = 0; i < dbg->num_breakpoints; ++i) {
            struct dbg_breakpoint *brk = &dbg->breakpoints[i];
            if (brk->used && brk->active && pc == brk->addr && !memcmp(segname, &brk->seg, 8)) {
                printf("Hit breakpoint %d at %.8s:0x%x\n", i, brk->seg.name, brk->addr);
                return true;
            }
        }
    } else {
        /* Clear these, so that the breakpoint will hit here next time */
        dbg->curerec = 0;
        dbg->curipc  = 0;
    }
    return false;
}

struct psys_debugger *psys_debugger_new(struct psys_state *s)
{
    struct psys_debugger *dbg = CALLOC_STRUCT(psys_debugger);
    dbg->state                = s;
    return dbg;
}

void psys_debugger_destroy(struct psys_debugger *dbg)
{
    free(dbg);
}
