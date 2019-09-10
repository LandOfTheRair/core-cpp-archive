#!/bin/bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CPUS="$( grep -c ^processor /proc/cpuinfo )"
export CFLAGS="-mavx -mavx2"
export CXXFLAGS="-mavx -mavx2"

cd $DIR/external/libsodium
./autogen.sh
./configure --enable-retpoline --enable-shared=no
make clean
make -j$CPUS

cd $DIR/external/libpqxx
./configure --disable-documentation
make clean
make -j$CPUS

cd $DIR/external/yaml-cpp
cmake -DCMAKE_BUILD_TYPE=Debug .
make clean
make -j$CPUS

cd $DIR/external/xxHash
make clean
make -j$CPUS
rm *.so*

cd $DIR
