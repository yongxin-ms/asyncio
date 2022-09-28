#!/bin/bash
basepath=$(cd `dirname $0`; pwd)
dir=$basepath/BuildRelease
mkdir -p "$dir"
cd "$dir"
#cmake "-DCMAKE_BUILD_TYPE=Release" "-DCMAKE_TOOLCHAIN_FILE=~/.vcpkg/scripts/buildsystems/vcpkg.cmake" ..
cmake "-DCMAKE_BUILD_TYPE=Release" ..
make -j8
#scl enable devtoolset-7 "make -j 16"