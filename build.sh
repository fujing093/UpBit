#!/bin/sh
rm -rf build
mkdir build
cd build
cmake -Dtest=OFF -DCMAKE_BUILD_TYPE=Release ..
make -j8
#make test
