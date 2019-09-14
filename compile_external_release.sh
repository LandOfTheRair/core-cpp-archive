#!/bin/bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CPUS="$( grep -c ^processor /proc/cpuinfo )"
export CFLAGS="-DXXH_CPU_LITTLE_ENDIAN=1 -DNDEBUG -mavx"
export CXXFLAGS="-DXXH_CPU_LITTLE_ENDIAN=1 -DNDEBUG -mavx"

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
cmake -DCMAKE_BUILD_TYPE=Release -DYAML_CPP_BUILD_CONTRIB=Off -DYAML_CPP_BUILD_TESTS=Off -DYAML_CPP_BUILD_TOOLS=Off .
make clean
make -j$CPUS

cd $DIR/external/xxHash
make clean
make -j$CPUS
rm *.so*

cd $DIR
