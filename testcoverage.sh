#!/bin/bash
#./configure --with-check --enable-gcov
make clean
make check

lcov --directory sockaddrutil/test/ --directory stunlib/test/ --directory icelib/test/ --capture --output-file natlib.info

genhtml --output-directory lcov --no-branch-coverage natlib.info