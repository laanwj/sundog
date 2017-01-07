Tools
===========

This project comes with tooling for analyzing p-system files. To use this, you
first need to extract the individual files from the image.

Extracting files
-------------------

To extract the files `SYSTEM.INTERP`, `SYSTEM.PASCAL`, `SYSTEM.STARTUP` and
`SYSTEM.MISCINFO` use the tool `ucsdpsys_disk` tool from
[ucsd-psystem-fs](http://ucsd-psystem-fs.sourceforge.net/):

```bash
mkdir files
ucsdpsys_disk -f sundog.st -g files
```

### Files

- `SYSTEM.INTERP` contains 68000 code for the interpreter (PME). This is not used
  by this project.
- `SYSTEM.PASCAL` contains the Pascal library and p-system OS. This is the first
  file that is located and loaded.
- `SYSTEM.STARTUP` contains the game itself.
- `SYSTEM.MISCINFO` contains miscelleneous configuration information. This is loaded
  by the p-system OS. Some of the parameters (those to do with memory) are overridden
  in our loader.

Only `SYSTEM.PASCAL` add `SYSTEM.STARTUP` contain p-code, and can be disassembled using
the following tool.

Disassembling
---------------

The main tool for disassembling and analyzing p-code files is `tools/list_pcode.py`.

Basic usage is:

```bash
tools/list_pcode.py files/SYSTEM.STARTUP | less -R
```

To dump a specific segment and procedure number use:

```bash
tools/list_pcode.py -s USERPROG:0x02 files/SYSTEM.PASCAL
```

A full list of options is available with:

```bash
tools/list_pcode.py --help
```

Other tools
-------------

There are a few miscellaneous single-purpose tools, most of them related to the
game, for example `extract_gfx.py` can be used to extract the font and mouse
cursors.
