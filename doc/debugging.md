Debugging
===================

Tracing
---------------

To trace every instruction executed in utmost detail, set the
environment variable:

```
PSYS_DEBUG=0xffff
```

Interactive debugger
---------------------

To go to the interactive debugger press `d` while the game is running. The
project needs to have been compiled with `-DPSYS_DEBUGGER` for that to work.

