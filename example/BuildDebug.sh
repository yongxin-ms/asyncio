#!/bin/bash
basepath=$(cd `dirname $0`; pwd)
dir=$basepath/build
mkdir -p "$dir"
cd "$dir"
#cmake "-DCMAKE_BUILD_TYPE=Debug" "-DCMAKE_TOOLCHAIN_FILE=~/.vcpkg/scripts/buildsystems/vcpkg.cmake" ..
cmake "-DCMAKE_BUILD_TYPE=Debug" ..
make -j8
#scl enable devtoolset-7 "make -j 16"
