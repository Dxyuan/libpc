#!/bin/sh

set -x

cd `dirname $0`
mkdir -p build
cd build
cmake ..
make && make install
cd ..
rm -rf build
