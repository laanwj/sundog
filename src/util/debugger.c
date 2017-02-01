/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "debugger.h"

#include "psys/psys_constants.h"
#include "psys/psys_state.h"
#include "psys/psys_debug.h"
#include "psys/psys_helpers.h"
#include "util/memutil.h"

#include <fcntl.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

/** Rudimentary p-system debugger. The following functionality would be nice:
 * - Load / set VM register (lpr/spr)
 * - Single-step (execution control in general - this will need the debugger to have state)
 * - Breakpoints (code, data)
 *   - Break on runtime fault / error
 * - Run until end of function
 * - Show static closure (static links above current stackframe)
 * - Access intermediate variables (lda, lod, str)
 * - Up/down stack frame (lda/lod/str lda/lla/stl) should apply to selected stackframe
 * - Call procedure (with arguments)
 * - Expression evaluation
 * - Switch task
 * - Change task priority
 * - List tasks (nontrivial, unlike for erecs there seems to be no linked directory of tasks)
 * - Create/compare execution trace
 * - Make resident/swap out segment
 * - Throw runtime fault / error
 * - Print heap statistics
 */

#define MAX_BREAKPOINTS 40

struct dbg_breakpoint {
    bool used;
    bool active;
    int num;
    /* Only one type of breakpoint for now:
     * by code address.
     */
    struct psys_segment_id seg; /* Segment name */
    psys_word addr; /* Address relative to segment */
};

struct psys_debugger {
    struct psys_state *state;
    /* Number of breakpoint entries used */
    int num_breakpoints;
    /* Breakpoints */
    struct dbg_breakpoint breakpoints[MAX_BREAKPOINTS];
};

/** Primitive argument parsing / splitting.
 * Separates out arguments separated by any number of spaces.
 */
static unsigned parse_args(char **args, unsigned max, char *line)
{
    char *arg = line;
    unsigned num = 0;
    if (!line)
        return 0;
    /* skip initial spaces */
    while (*arg == ' ') {
        ++arg;
    }
    /* split arguments */
    while(arg && *arg && num < max) {
        char *nextarg = strchr(arg, ' ');
        args[num++] = arg;
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

/* Get initial erec pointer from KERNEL. From there the list of erecs can be
 * traversed.
 */
static psys_fulladdr first_erec_ptr(struct psys_state *s)
{
    return psys_ldw(s, 0x14e);
}

/** Assign a segment name. Names are case-insensitive, like Pascal.
 * Names longer than 8 characters will be truncated.
 */
static void assign_segment_id(struct psys_segment_id *id, const char *name)
{
    unsigned i;
    /* Make truncated, uppercased, space-padded version of name to compare to */
    for (i=0; i<8 && name[i]; ++i) {
        id->name[i] = toupper(name[i]);
    }
    for (; i<8; ++i) {
        id->name[i] = ' ';
    }
}

/** Get erec for segment by name. Names are case-insensitive, like Pascal.
 */
static psys_fulladdr get_erec_for_segment(struct psys_state *s, const char *name)
{
    psys_word erec = first_erec_ptr(s);
    struct psys_segment_id id;
    assign_segment_id(&id, name);
    /* Traverse linked list of erecs */
    while (erec) {
        psys_word sib = psys_ldw(s, erec + PSYS_EREC_Env_SIB);
        if (!memcmp(id.name, psys_bytes(s, sib + PSYS_SIB_Seg_Name), 8)) {
            return erec;
        }
        erec = psys_ldw(s, erec + PSYS_EREC_Next_Rec);
    }
    return PSYS_NIL;
}

/** Get globals base and size for segment by name */
static psys_fulladdr get_globals(struct psys_state *s, char *name, psys_word *data_size)
{
    psys_fulladdr erec = get_erec_for_segment(s, name);
    if (erec == PSYS_NIL) {
        if (data_size) {
            *data_size = 0;
        }
        return erec;
    }
    if (data_size) {
        psys_word sib = psys_ldw(s, erec + PSYS_EREC_Env_SIB);
        *data_size = psys_ldw(s, sib + PSYS_SIB_Data_Size);
    }
    return psys_ldw(s, erec + PSYS_EREC_Env_Data);
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
    for (i=0; i<MAX_BREAKPOINTS; ++i) {
        struct dbg_breakpoint *brk = &dbg->breakpoints[i];
        if (!brk->used) {
            /* Adjust num_breakpoints to include the newly allocated slot */
            if (i >= dbg->num_breakpoints) {
                dbg->num_breakpoints = i + 1;
            }
            brk->used = true;
            brk->num = i;
            return brk;
        }
    }
    return NULL;
}

/** Maximum number of arguments for any debugger command.
 */
#define MAX_ARGS (10)

void psys_debugger_run(struct psys_debugger *dbg, bool user)
{
    struct psys_state *s = dbg->state;
    printf("** Entering psys debugger: type 'h' for help **\n");
    while (true) {
        char *args[MAX_ARGS];
        char *cmd;
        unsigned num;

        char *line = readline("\x1b[38;5;220mpsys\x1b[38;5;235m>\x1b[38;5;238m>\x1b[38;5;242m>\x1b[0m ");
        if (line && *line && *line!=' ') {
            add_history(line);
        }
        /* num is number of tokens, including command */
        num = parse_args(args, MAX_ARGS, line);

        if (num) {
            cmd = args[0];

            if (!strcmp(cmd, "q")) {
                exit(1);
            } else if (!strcmp(cmd, "b")) { /* Set breakpoint */
                if (num >= 3) {
                    struct dbg_breakpoint *brk = new_breakpoint(dbg);
                    assign_segment_id(&brk->seg, args[1]);
                    brk->addr = strtol(args[2], NULL, 0);
                    brk->active = true;
                    printf("Set breakpoint %d at %.8s:0x%x\n", brk->num, brk->seg.name, brk->addr);
                } else {
                    printf("Two arguments required\n");
                }
            } else if (!strcmp(cmd, "db") || !strcmp(cmd, "eb")) { /* Disable or enable breakpoint */
                if (num >= 2) {
                    int i = strtol(args[1], NULL, 0);
                    bool enable = !strcmp(cmd, "eb");
                    if (i>=0 && i<dbg->num_breakpoints) {
                        struct dbg_breakpoint *brk = &dbg->breakpoints[i];
                        brk->active = enable;
                        printf("%s breakpoint %d\n", enable?"Enabled":"Disabled", brk->num);
                    } else {
                        printf("Breakpoint %i out of range\n", i);
                    }
                } else {
                    printf("One argument required\n");
                }
            } else if (!strcmp(cmd, "lb")) { /* List breakpoints */
                int i;
                printf("\x1b[38;5;202;48;5;235m>   Breakpoints   >\x1b[0m\n");
                for (i=0; i<dbg->num_breakpoints; ++i) {
                    struct dbg_breakpoint *brk = &dbg->breakpoints[i];
                    if (brk->used) {
                        printf("%2d %d at %.8s:0x%x\n", i, brk->active, brk->seg.name, brk->addr);
                    }
                }
            } else if (!strcmp(cmd, "l")) {
                psys_word erec = first_erec_ptr(s);
                printf("\x1b[38;5;202;48;5;235merec sib  flg segname  base size \x1b[0m\n");
                while (erec) {
                    psys_word sib = psys_ldw(s, erec + PSYS_EREC_Env_SIB);
                    psys_word data_base = psys_ldw(s, erec + PSYS_EREC_Env_Data); /* globals start */
                    psys_word data_size = psys_ldw(s, sib + PSYS_SIB_Data_Size) * 2; /* globals size in bytes */
                    psys_word evec = psys_ldw(s, erec + PSYS_EREC_Env_Vect);
                    psys_word num_evec = psys_ldw(s, W(evec, 0));

                    unsigned i;
                    /* Print main segment */
                    printf("%04x   %04x %c %-8.8s %04x %04x\n",
                            erec, sib,
                            is_segment_resident(s, erec) ? 'R':'-',
                            psys_bytes(s, sib + PSYS_SIB_Seg_Name),
                            data_base, data_size
                            );
                    /* Subsidiary segments. These will be referenced in the segment's evec
                     * and have the same evec pointer (and the same BASE, but that's less reliable
                     * as some segments have no globals).
                     */
                    for (i=1; i<num_evec; ++i) {
                        psys_word serec = psys_ldw(s, W(evec,i));
                        if (serec) {
                            psys_word ssib = psys_ldw(s, serec + PSYS_EREC_Env_SIB);
                            psys_word sevec = psys_ldw(s, serec + PSYS_EREC_Env_Vect);
                            if (serec != erec && sevec == evec) {
                                printf("  %04x %04x %c %-8.8s\n",
                                        serec, ssib,
                                        is_segment_resident(s, serec) ? 'R':'-',
                                        psys_bytes(s, ssib + PSYS_SIB_Seg_Name)
                                        );
                            }
                        }
                    }
                    erec = psys_ldw(s, erec + PSYS_EREC_Next_Rec);
                }
            } else if (!strcmp(cmd, "bt")) {
                psys_print_traceback(s);
            } else if (!strcmp(cmd, "c")) {
                free(line);
                break;
            } else if (!strcmp(cmd, "s")) {
                if (num >= 2) {
                    int fd = open(args[1], O_CREAT|O_WRONLY, 0666);
                    int rv = write(fd, s->memory, s->mem_size);
                    (void)rv;
                    close(fd);
                    printf("Wrote memory dump to %s\n", args[1]);
                } else {
                    printf("Two arguments required\n");
                }
            } else if (!strcmp(cmd, "x")) { /* examine */
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
                    for (i=2; i<num; ++i) {
                        psys_stw(s, addr + (i-2)*2, strtol(args[i], NULL, 0));
                    }
                } else {
                    printf("At least two arguments required\n");
                }
            } else if (!strcmp(cmd, "wb")) { /* write byte */
                if (num >= 3) {
                    unsigned addr = strtol(args[1], NULL, 0);
                    unsigned i;
                    for (i=2; i<num; ++i) {
                        psys_stb(s, addr, i-2, strtol(args[i], NULL, 0));
                    }
                } else {
                    printf("At least two arguments required\n");
                }
            } else if (!strcmp(cmd, "sro")) {
                if (num >= 4) {
                    psys_word data_size;
                    psys_fulladdr data_base = get_globals(s, args[1], &data_size);
                    unsigned addr = strtol(args[2], NULL, 0);
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
            } else if (!strcmp(cmd, "ldo") || !strcmp(cmd, "lao")) {
                if (num == 3) {
                    psys_word data_size;
                    psys_fulladdr data_base = get_globals(s, args[1], &data_size);
                    unsigned addr = strtol(args[2], NULL, 0);
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
            } else if (!strcmp(cmd, "stl")) {
                if (num >= 3) {
                    unsigned addr = strtol(args[1], NULL, 0);
                    psys_stw(s, W(s->mp + PSYS_MSCW_VAROFS, addr),
                            strtol(args[2], NULL, 0));
                } else {
                    printf("At least two arguments required\n");
                }
            } else if (!strcmp(cmd, "ldl") || !strcmp(cmd, "lla")) {
                if (num == 2) {
                    unsigned addr = strtol(args[1], NULL, 0);
                    if (!strcmp(cmd, "lao")) {
                        psys_word address = W(s->mp + PSYS_MSCW_VAROFS, addr);
                        printf("0x%04x\n", address);
                    } else {
                        psys_word value = psys_ldw(s, W(s->mp + PSYS_MSCW_VAROFS, addr));
                        printf("0x%04x\n", value);
                    }
                } else {
                    printf("One argument is required\n");
                }
            } else if (!strcmp(cmd, "r")) {
                 psys_print_info(s);
            } else if (!strcmp(cmd, "h") || !strcmp(cmd, "?")) {
                printf("\x1b[38;5;202;48;5;235m>   Help   >\x1b[0m\n");
                printf("b <seg> <addr>      Set breakpoint at seg:addr\n");
                printf("db <i>              Disable breakpoint i\n");
                printf("eb <i>              Enable breakpoint i\n");
                printf("lb                  List breakpoints\n");
                printf("bt                  Show backtrace\n");
                printf("c                   Continue execution\n");
                printf("lao <seg> <g>       Show address of global variable\n");
                printf("sro <seg> <g> <val> Set global variable\n");
                printf("ldo <seg> <g>       Examine global variable\n");
                printf("lla <l>             Show address of local variable\n");
                printf("stl <l> <val>       Set local variable\n");
                printf("ldl <l>             Examine local variable\n");
                printf("h                   This information\n");
                printf("l                   List environments\n");
                printf("q                   Quit\n");
                printf("r                   Show registers and current instruction\n");
                printf("s <file>            Write entire p-system memory to file\n");
                printf("wb <addr> <val> ... Write byte(s) to address\n");
                printf("ww <addr> <val> ... Write word(s) to address\n");
                printf("x <addr>            Examine memory\n");
                printf("\n");
                printf("All values can be specified as decimal or 0xHEX.\n");
            } else {
                printf("Unknown command %s. Type 'h' for help.\n", cmd);
            }
        } else {
            if (!line) {
                /* EOF / Ctrl-D */
                exit(1);
            }
        }

        free(line);
    }

}

/** Called for every instruction - this should break on breakpoints */
bool psys_debugger_trace(struct psys_debugger *dbg)
{
    struct psys_state *s = dbg->state;
    psys_word sib = psys_ldw(s, s->erec + PSYS_EREC_Env_SIB);
    const psys_byte *segname = psys_bytes(s, sib + PSYS_SIB_Seg_Name);
    psys_word pc = s->ipc - s->curseg; /* PC relative to current segment */
    int i;
    for (i=0; i<dbg->num_breakpoints; ++i) {
        struct dbg_breakpoint *brk = &dbg->breakpoints[i];
        if (brk->used && brk->active && pc == brk->addr && !memcmp(segname, &brk->seg, 8)) {
            printf("Hit breakpoint %d at %.8s:0x%x\n", i, brk->seg.name, brk->addr);
            return true;
        }
    }
    return false;
}

struct psys_debugger *psys_debugger_new(struct psys_state *s)
{
    struct psys_debugger *dbg = CALLOC_STRUCT(psys_debugger);
    dbg->state = s;
    return dbg;
}

void psys_debugger_destroy(struct psys_debugger *dbg)
{
    free(dbg);
}
