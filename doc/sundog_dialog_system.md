Sundog dialog system
========================

The game contains a quite comphrehensive scripted dialog system,
for games of the time.

It is a clever combination of compactness and expressiveness.

At the lowest level there are 32 "stock phrases" which
are represented as one byte. Some of these are hardcoded,
others are handled specially by the game based on the context.

There can be multiple alternatives within a phrase, by default
these are select at random to give some variation, but the script
can also select a specific variant.

Dumping
------------
The tool [tools/dump\_text.py](tools/dump_text.py) can be used to dump all the
dialog text of the game in a human-readable format.

Stock phrases
------------------

These are handled in `XDOINTER:0x11`, and occupy the character range
`0x00..0x3f`. Most of the phrases are supplied in the initial two blocks of the
dialog data. However,

- `0x20`..`0x2f` are handled specially, as follows:

```
0x20  Passed through as ASCII space
0x21  (nop, should be "!")
0x22  "O.K."
0x23  XDOINTER:0x0f
0x24  Formatted money amount (XDOINTER_01_L4c)
0x25  Name of item (XDOINTER_01_L54)
0x26  "we" "I"
0x27  Passed through as ASCII "'"
0x28  "us" "me"
0x29  " or not"
0x2a  "what"
0x2b  "you"
0x2c  Passed through as ASCII ","
0x2d  "[13]|[13]|[32]|[32]|[31]"
0x2e  (nop, should be ".")
0x2f  "buy" "sell"
```

- `0x34..0x3e` are specific to a dialog block.
- `0x3f` seems to have a special purpose too, it is always at the end of the text.

Control flow
--------------

At the top-level, XDOINTER:0x01 is called with three arguments.
Likely they determine the context of the dialogue and the
person.

- Example (bar): 0x000f, 0x0005, 0x000c
- Example (hotel): 0x000f, 0x0005, 0x0017

Last parameter seems to be the category:

```
0x0c    Beer + Burger (Bar)
0x0d    Rapidheal + Shield (Weapons shop)
0x0e    Ship parts shop
0x0f    "A quick search turns up XXX Cr"
0x10    "Hand over your cash or die"
0x11    "I am told you wish to buy a peptab"
0x12    "Can you spare 7 Cr for a poor beggar"
0x13    "Traffic control here"
0x14
0x15
0x16
0x17    "Looking for a room?"
```

Debugging
```
    b XDOINTER 0x16fe
    c
    ldl 0x364
    ldl 0x365
    ldl 0x366
```

Internally, categories are mapped to dialog blocks.
These are just the entry points; dialog blocks can refer to other blocks,
to continue the dialog:
```
Cat.   Dialog block (XDOINTER:0x1f)
0x0c   (0x0010)
0x11   (0x0020)
?      (0x0021)
?      (0x0030)
?      (0x0031)
0x13   (0x0032)
0x0f   (0x0033)
?      (0x0040)
0x10   (0x0041)
0x0d   (0x0050)
0x0e   (0x0051)
0x17   (0x0060)
?      (0x00c0)  "How many repair techs does it take..." (jokes)
?      (0x00d0)  "Obviously you're not serious..."
?      (0x00e0)  "Hi! my name is *, I wrote the * for this game. Pretty great game, huh?"
?      (0x00e1)  "Glad you like it! Oh by the way, here's ..."
?      (0x00e2)  "Well, you've got no class..."
?      (0x0004)  When exiting a dialog
?      (0x0000)  When exiting a dialog
```
These match the dialog block numbers (per group of *16*, as that's how they are
organized into disk sectors) that *tools/dump\_text.py* shows.

