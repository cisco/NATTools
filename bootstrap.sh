#!/bin/sh
cd icelib
./bootstrap.sh
cd ../stunlib
./bootstrap.sh
cd ../sockaddrutil
./bootstrap.sh
mkdir m4 build-aux
cd ..
autoreconf --install

