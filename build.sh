#!/bin/sh
set -euo pipefail

mkdir -p build

clang++ -std=c++23 -O3 -DNDEBUG -march=native -mcpu=native -pthread \
-flto=thin -fvisibility=hidden -fno-rtti -fno-exceptions -fomit-frame-pointer \
-Wall -Wextra -pedantic \
idioten.cc -o build/idioten

./build/idioten
