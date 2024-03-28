# Atlantis
Atlantis CBLS solver

## Building
The project is built using cmake, but a `Makefile` is used to simplify the execution of cmake.

### First time
- `make submodule` to initialise the submodules (used for testing and benchmarking).
- `make` to build
- `make run` to see if things are working.


### Building Queens

```sh
cmake --build build --config Release --target queens
```

### Setup dev environment
The project should now be importable as a cmake project.


## Before making public
Make sure to purge files from git history that may have contrained tokens, such as travis.yml
Or just create a new public repot with no history....

https://docs.github.com/en/free-pro-team@latest/github/authenticating-to-github/removing-sensitive-data-from-a-repository



## Resources

C++ tools: https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md
