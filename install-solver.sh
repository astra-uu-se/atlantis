#!/bin/bash

# Build the project
cmake --build build --target cbls

# Copy the executable to the minizinc directory.
cp build/cbls minizinc/astra-cbls
printf "\nCopied executable to solver folder.\n"

# Modify path
export MZN_SOLVER_PATH="$PWD/minizinc"
echo "Added solver folder to MZN_SOLVER_PATH."