#!/bin/bash

#stop on first error result
set -e

STD=20

function usage {
    echo ""
    echo "maketest.sh [--std <20 | 17>]"
    echo ""
    echo "  --std    Select the C++ standard to build for."
    echo "           Default: 20."
    echo ""
}


while [[ $# -gt 0 ]]
do
    case $1 in
        --std)
            STD=$2
            shift
            shift
            ;;
        *)
            usage
            exit
            ;;
    esac
done

STDARG="--std=c++${STD}"

echo "-----------------------------------------------------"
echo "Building test.cpp for ${STDARG}"

time clang++ -O3 ${STDARG} -Wall -Wextra -I./include test/test.cpp -o bin/test


echo ""
echo "-----------------------------------------------------"
echo "Running Tests"

time bin/test



