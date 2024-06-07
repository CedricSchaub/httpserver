#!/bin/bash

# compiler and linker flags
# yeah it's named compiler flags <.<
COMPILER_FLAGS=("-g" "-Wall" "-Wextra" "-Wfloat-equal" "-Wundef" "-Wshadow" "-Wcast-align" "-Wstrict-prototypes" "-Wswitch-default" "-Wswitch-enum" "-Wconversion")

if ! [[ -d "./build" ]]; then
	mkdir "./build"
fi

echo "About to build main.c\n";
gcc "main.c" "args.c" -o "build/main" "${COMPILER_FLAGS[@]}"  && ./build/main "${@}"
