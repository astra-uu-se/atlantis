#!/bin/bash

# Build the project
cmake --build build

# Copy the executable to the minizinc directory.
cp build/cbls minizinc/astra-cbls

# Modify path
export MZN_SOLVER_PATH="$PWD/minizinc"