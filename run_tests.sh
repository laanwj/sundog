#!/bin/sh
set -ex
TESTDIR=../build/src/test
cd src
${TESTDIR}/set_tests
${TESTDIR}/inst_tests
${TESTDIR}/img_tests
echo "OK"
