#include "game/game_debug.h"

#include "util/memutil.h"

static const struct util_debuginfo_entry game_debuginfo_entries[] = {
#include "game/game_debuginfo.h"
};

static const struct util_debuginfo game_debuginfo = {
    game_debuginfo_entries,
    ARRAY_SIZE(game_debuginfo_entries)
};

const struct util_debuginfo *get_game_debuginfo(void)
{
    return &game_debuginfo;
}
