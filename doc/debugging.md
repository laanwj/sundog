Debugging
===================

Tracing
---------------

To trace every instruction executed in utmost detail, set the
environment variable:

```
PSYS_DEBUG=0xffff
```

Individual bits can be set or cleared to enable specific
classes of debug logging. See [psys_debug.h](../src/psys/psys_debug.h)
for the list.

Interactive debugger
---------------------

To go to the interactive debugger press `d` while the game is running. The
project needs to have been compiled with `-DPSYS_DEBUGGER` for that to work.

In the debugger type `h` for a list of commands.

Cheats
--------

- Pressing `x` in game will (for now) give you virtually unlimited funds and
  stash the ship's stores with one of every possible item.
