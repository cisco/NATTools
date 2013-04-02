#!/bin/bash
#./configure --with-check --enable-gcov

make clean
make check

lcov --directory test/ --capture --output-file icelib.info


genhtml --output-directory lcov --no-branch-coverage icelib.info

firefox -new-tab lcov/index.html