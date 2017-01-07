/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "game_shiplib.h"

#include "psys/psys_debug.h"
#include "psys/psys_helpers.h"
#include "psys/psys_state.h"
#include "util/util_save_state.h"

#include "util/memutil.h"

#include <string.h>

/* Header for savestates */
#define GAME_SHIPLIB_STATE_ID 0x53484950

#define SHIPLIB_NUM_PROC 0x26

struct shiplib_priv {
};

static void shiplib_16(struct psys_state *s, struct shiplib_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_debug("shiplib_16 (stub)\n");
}

static void shiplib_18(struct psys_state *s, struct shiplib_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_debug("shiplib_18 (stub)\n");
}

static void shiplib_19(struct psys_state *s, struct shiplib_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word x = psys_pop(s);
    psys_debug("shiplib_19 0x%04x (stub)\n", x);
}

static void shiplib_1A(struct psys_state *s, struct shiplib_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word d = psys_pop(s);
    psys_word c = psys_pop(s);
    psys_word b = psys_pop(s);
    psys_word a = psys_pop(s);
    psys_debug("shiplib_1A 0x%04x 0x%04x 0x%04x 0x%04x (stub)\n", a, b, c, d);
}

static int shiplib_save_state(struct psys_binding *b, int fd)
{
    uint32_t id = GAME_SHIPLIB_STATE_ID;
    /* Save shiplib state (dummy) */
    if (FD_WRITE(fd, id)) {
        return -1;
    }
    return 0;
}

static int shiplib_load_state(struct psys_binding *b, int fd)
{
    uint32_t id;
    /* Load shiplib state (dummy) */
    if (FD_READ(fd, id)) {
        return -1;
    }
    if (id != GAME_SHIPLIB_STATE_ID) {
        psys_debug("Invalid shiplib state record %08x\n", id);
        return -1;
    }
    return 0;
}

struct psys_binding *new_shiplib(struct psys_state *state, struct game_screen *screen)
{
    struct psys_binding *b    = CALLOC_STRUCT(psys_binding);
    struct shiplib_priv *priv = CALLOC_STRUCT(shiplib_priv);
    (void)state;
    b->userdata     = priv;
    b->num_handlers = SHIPLIB_NUM_PROC;
    memcpy(b->seg.name, "SHIPLIB ", 8);
    b->handlers       = calloc(SHIPLIB_NUM_PROC, sizeof(psys_bindingfunc *));
    b->handlers[0x16] = (psys_bindingfunc *)&shiplib_16;
    b->handlers[0x18] = (psys_bindingfunc *)&shiplib_18;
    b->handlers[0x19] = (psys_bindingfunc *)&shiplib_19;
    b->handlers[0x1a] = (psys_bindingfunc *)&shiplib_1A;

    b->save_state = &shiplib_save_state;
    b->load_state = &shiplib_load_state;

    return b;
}

void destroy_shiplib(struct psys_binding *b)
{
    struct shiplib_priv *priv = (struct shiplib_priv *)b->userdata;
    free(b->handlers);
    free(priv);
    free(b);
}
