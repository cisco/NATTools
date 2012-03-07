#!/bin/bash
#./configure --with-check --enable-gcov

make clean
make check

lcov --directory test/ --capture --output-file sockaddrutil.info


genhtml --output-directory lcov --no-branch-coverage sockaddrutil.info

firefox -new-tab file:///home/palerikm/development/sockaddrutil/lcov/index.html