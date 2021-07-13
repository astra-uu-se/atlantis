export CMAKE_OPTIONS+= ${ENV_CMAKE_OPTIONS}

CMAKE=$(shell which cmake)
MKFILE_PATH=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR=${MKFILE_PATH}build

.PHONY: clean
clean:
	rm -rf ${BUILD_DIR}

.PHONY: build
build:
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR}; \
	$(CMAKE) ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=Release ..; \
	$(MAKE)

.PHONY: build-tests
build-tests:
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR}; \
	$(CMAKE) ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS:BOOL=ON ..; \
	$(MAKE)

.PHONY: build-benchmarks
build-benchmarks:
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR}; \
	$(CMAKE) ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS:BOOL=ON ..; \
	$(MAKE)

.PHONY: run
run: build
	exec ${BUILD_DIR}/cbls

.PHONY: run-tests
run-tests: build-tests
	exec ${BUILD_DIR}/runUnitTests

.PHONY: run-benchmarks
run-benchmarks: build-benchmarks
	exec ${BUILD_DIR}/runBenchmarks

.PHONY: all
all: clean build build-tests build-benchmarks