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

These are handled in `XDOINTER:0x11`.

`0x20`..`0x2f` are handled specially.

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
