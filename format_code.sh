#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

clang-format -style=file -i \
    ${DIR}/src/*.[ch] \
    ${DIR}/src/{test,psys,game,util}/*.[ch]
