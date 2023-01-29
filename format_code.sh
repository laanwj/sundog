#!/bin/sh
set -e

find src -iname \*.c -o -iname \*.h -o -iname \*.cpp | xargs clang-format -style=file -i
