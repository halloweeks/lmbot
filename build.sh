#!/bin/sh

set -e

clear

if [ ! -d build ]; then
    mkdir build
fi

cd build

cmake ..
make -j4

cp ./client $HOME/
chmod +x $HOME/client