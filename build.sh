#!/bin/bash

FILE="${1:-main.c}"
FILE_BASENAME_NO_EXTENSION=$(basename -s ".c" "${FILE}")

ARGS=()
# compiler and linker flags
# yeah it's named compiler flags <.<
COMPILER_FLAGS=("-g" "-Wall" "-Wextra" "-Wfloat-equal" "-Wundef" "-Wshadow" "-Wcast-align" "-Wstrict-prototypes" "-Wswitch-default" "-Wswitch-enum" "-Wconversion")

if ! [[ -z "${2}" ]]; then
	ARGS+=("${@}")
fi

if ! [[ -d "./build" ]]; then
	mkdir "./build"
fi

echo "About to build main.c\n";
gcc "${FILE}" -o "build/${FILE_BASENAME_NO_EXTENSION}" "${COMPILER_FLAGS[@]}"  && ./build/${FILE_BASENAME_NO_EXTENSION} "${ARGS[@]}"
