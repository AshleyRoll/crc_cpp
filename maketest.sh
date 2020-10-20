#!/bin/bash

#stop on first error result
set -e

STD=20

function usage {
    echo ""
    echo "maketest.sh [--std <20 | 17>] [--clang] [--gcc]"
    echo ""
    echo "  --std    Select the C++ standard to build for."
    echo "           Default: ${STD}."
    echo "  --clang  Compile with clang (default)"
    echo "  --gcc    Compile with gcc"
    echo "           Note: last parameter of --clang or --gcc is applied"
    echo ""
}

USECLANG=1

while [[ $# -gt 0 ]]
do
    case $1 in
        --std)
            STD=$2
            shift
            shift
            ;;
        --clang)
            USECLANG=1
            shift
            ;;
        --gcc)
            USECLANG=0
            shift
            ;;
        *)
            usage
            exit
            ;;
    esac
done

STDARG="--std=c++${STD}"
COMPILER=clang++
[[ ${USECLANG} -eq 0 ]] && COMPILER=g++


BIN="bin"
OUT="${BIN}/test-${COMPILER}${STD}"

echo "-----------------------------------------------------"
echo "Building test.cpp with ${COMPILER} ${STDARG}"

mkdir -p ${BIN}

time clang++ -O3 "${STDARG}" -Wall -Wextra -I./include test/test.cpp -o "${OUT}"


echo ""
echo "-----------------------------------------------------"
echo "Running Tests"

time "${OUT}"



