# Atlantis
Atlantis CBLS solver

## Building
The project is built using cmake, but a `Makefile` is used to simplify the execution of cmake.
This project is tested with gcc version 13.

### First time
- `make` to build

### Building Queens

```sh
cmake --build build --config Release --target queens
```

### Setup dev environment
The project should now be importable as a cmake project.


## Resources

C++ tools: https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md

## MiniZinc Challenge Tooling

- [MiniZinc challenge sweep workflow](test/sweep-challenge/README.md)
- [Challenge compatibility patch packs](test/sweep-challenge/challenge-patches/README.md)
