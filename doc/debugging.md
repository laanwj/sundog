Debugging
===================

Debug UI
--------------

When compiled with `ENABLE_DEBUGUI` compiler flag, a imgui-based debug UI is
compiled in. This is currently experimental and in development, but it allows
editing memory as well as checking the current FPS rate and the palette.

It can be shown/hidden with `\``/`~` (as in, the console key in Quake).
Different windows can be opened from there: segments, memory and palette.
"test window" is the imgui test window and only useful to look at imgui's
capabilities.

Tracing
---------------

To trace every instruction executed in utmost detail, set the
environment variable:

```
PSYS_DEBUG=0xffff
```

Individual bits can be set or cleared to enable specific
classes of debug logging. See [psys_debug.h](../src/psys/psys_debug.h)
for the list. For example, to show nested procedure call tracing a la *ltrace*
use:

```
PSYS_DEBUG=0x80
```

Example output:
```
[006e]       XSTARTUP:0x01:2e11 → XSTARTUP:0x46  ()
[006e]        XSTARTUP:0x46:26a6 → XSTARTUP:0x48 
[006e]         XSTARTUP:0x48:2669 → MAINLIB :0x4b 
[006e]          MAINLIB :0x4b:12df → MAINLIB :0x49  (0x0001, 0x00ff, 0x0000, 0xe34a, 0x0001, 0x005b, 0x0001)
[006e]           MAINLIB :0x49:0ff6 → MAINLIB :0x2d  ()
[006e]           MAINLIB :0x49:1016 → MAINLIB :0x8  (0x005b)
[006e]           MAINLIB :0x49:1085 → MAINLIB :0x54  ()
[006e]           MAINLIB :0x49:109b → KERNEL  :0x12  (0x0004, 0xe34a, 0x0000, 0x0200, 0x005b, 0x0000)
[006e]           MAINLIB :0x49:10b0 → MAINLIB :0x45  ()
```

A list of known procedures can be found in [tools/appcalls\_list.py](../tools/appcalls_list.py) for
the game and [tools/libcalls\_list.py](../tools/libcalls_list.py) for the p-system library respectively.

Interactive debugger
---------------------

To go to the interactive debugger press `d` while the game is running. The
project needs to have been compiled with `-DPSYS_DEBUGGER` for that to work.

In the debugger type `h` for a list of commands.

Cheats
--------

- Pressing `x` in game will (for now) give you virtually unlimited funds and
  stash the ship's stores with one of every possible item.
- Pressing `y` in game will print the 512-byte gamestate as hexdump. By comparing this
  output this could be used to develop further cheats.
