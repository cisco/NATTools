#!/bin/bash
#./configure --with-check --enable-gcov

make clean
make check

lcov --directory test/ --capture --output-file stunlib.info


genhtml --output-directory lcov --no-branch-coverage stunlib.info

firefox -new-tab lcov/index.html