#!/bin/bash
# Exercise all the analysis
set -e
FILES="files/SYSTEM.PASCAL files/SYSTEM.STARTUP"
tools/list_pcode.py -m 0 $FILES > /dev/null
tools/list_pcode.py -m 1 $FILES > /dev/null
tools/list_pcode.py -m 2 $FILES > /dev/null
tools/list_pcode.py -m 3 $FILES > /dev/null
tools/list_pcode.py -m 4 $FILES > /dev/null
tools/list_pcode.py -m 5 $FILES > /dev/null
tools/list_pcode.py -m 6 $FILES > /dev/null
tools/list_pcode.py -m 7 $FILES > /dev/null
