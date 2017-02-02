/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_PSYS_INTERPRETER
#define H_PSYS_INTERPRETER

#include "psys_state.h"

/* call a function (invoke psys interpreter) */
extern void psys_call(struct psys_state *state, const struct psys_function_id id);

/* interpreter loop, runs until a binding sets running=false.
 */
extern void psys_interpreter(struct psys_state *state);

/* stop interpreter loop */
extern void psys_stop(struct psys_state *state);

/* raise execution error */
extern void psys_execerror(struct psys_state *s, psys_word err);

/* raise fault */
extern void psys_fault(struct psys_state *s, psys_fulladdr tib, psys_fulladdr erec, psys_fulladdr words, psys_word type);

/* Internal helper: Get from pool structure to address in memory */
psys_fulladdr psys_pool_get_base(struct psys_state *s, psys_fulladdr pool);

/* Internal helper: Look up the segment base address and add offset, given an
 * erec. Fault if a segment is not resident (and *fault* is set), and return
 * PSYS_ADDR_ERROR.
 */
psys_fulladdr psys_segment_from_erec(struct psys_state *s, psys_fulladdr erec, bool fault);

/* Internal helper: increase timestamp for swapping.
 * erec is the erec being exited.
 */
void psys_increase_timestamp(struct psys_state *state, psys_fulladdr erec);

/* Internal helper: Look up procedure address from erec and procedure number.
 * If the procedure is not resident raise a fault (if *fault* is set), and return
 * PSYS_ADDR_ERROR.
 */
psys_fulladdr lookup_procedure(struct psys_state *s, psys_fulladdr erec, psys_word procedure,
    bool fault, psys_word *num_locals_out, psys_word *end_pointer, psys_fulladdr *segment_out);

/* Look up EREC for segment number in local segment environment (EVEC).
 * Returns PSYS_ADDR_ERROR and set executin error (if *fault* set) if segment
 * not found.
 */
psys_fulladdr psys_lookup_ref_segment(struct psys_state *s, psys_word segid, bool fault);

#endif
