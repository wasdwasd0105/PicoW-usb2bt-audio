#!/bin/sh

echo "Configure and make" &&
cmake -B build -S . &&
make -j4 -C build