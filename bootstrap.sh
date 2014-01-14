#!/bin/sh
mkdir m4 build-aux
autoreconf --install
cd icelib
./bootstrap.sh
cd ../stunlib
./bootstrap.sh
cd ../sockaddrutil
./bootstrap.sh


