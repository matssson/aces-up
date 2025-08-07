#!/bin/sh
set -euo pipefail

mkdir -p build

clang++ -std=c++23 -O3 -pthread -Wall -Wextra -pedantic \
-flto -fvisibility=hidden -fno-exceptions -fomit-frame-pointer \
idioten.cc -o build/idioten

./build/idioten
