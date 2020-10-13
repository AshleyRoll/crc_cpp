#!/bin/bash

#stop on first error result
set -e

echo "Building test.cpp"

clang++ -O3 --std=c++20 -Wall -Wextra -I./include test/test.cpp -o bin/test

echo "Running Tests"
bin/test



