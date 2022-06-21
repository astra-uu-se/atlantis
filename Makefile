export CMAKE_OPTIONS+= ${ENV_CMAKE_OPTIONS}

CMAKE=$(shell which cmake)
MKFILE_PATH=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR=${MKFILE_PATH}build
MZN_MODEL_DIR=${MKFILE_PATH}mzn-models
FZN_MODEL_DIR=${MKFILE_PATH}fzn-models

export MZN_SOLVER_PATH=${MKFILE_PATH}minizinc

.PHONY: clean
clean:
	rm -rf ${BUILD_DIR}

.PHONY: build
build:
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR}; \
	$(CMAKE) ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS:BOOL=OFF -DBUILD_BENCHMARKS:BOOL=OFF ..; \
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
	exec ${BUILD_DIR}/atlantis

.PHONY: run-tests
run-tests: build-tests
	exec ${BUILD_DIR}/runUnitTests

.PHONY: run-benchmarks
run-benchmarks: build-benchmarks
	exec ${BUILD_DIR}/runBenchmarks

.PHONY: all
all: clean build build-tests build-benchmarks

.PHONY: fzn
fzn: #build
	cp build/atlantis minizinc/atlantis
	minizinc ${MZN_SOLVER_PATH}/atlantis.mpc -c \
					 ${MZN_MODEL_DIR}/car_sequencing.mzn \
					 ${MZN_MODEL_DIR}/car_sequencing.dzn \
					 --fzn ${FZN_MODEL_DIR}/car_sequencing.fzn
	rm -f ${MZN_MODEL_DIR}/car_sequencing.ozn
	minizinc ${MZN_SOLVER_PATH}/atlantis.mpc -c \
					 ${MZN_MODEL_DIR}/magic_square.mzn \
					 -D n=3 \
					 --fzn ${FZN_MODEL_DIR}/magic_square.fzn
	rm -f ${MZN_MODEL_DIR}/magic_square.ozn
	minizinc ${MZN_SOLVER_PATH}/atlantis.mpc -c \
					 ${MZN_MODEL_DIR}/tsp_alldiff.mzn \
					 ${MZN_MODEL_DIR}/tsp_17.dzn \
					 --fzn ${FZN_MODEL_DIR}/tsp_alldiff.fzn
	rm -f ${MZN_MODEL_DIR}/tsp_alldiff.ozn
	minizinc ${MZN_SOLVER_PATH}/atlantis.mpc -c \
					 ${MZN_MODEL_DIR}/n_queens.mzn \
					 -D n=32 \
					 --fzn ${FZN_MODEL_DIR}/n_queens.fzn
	rm -f ${MZN_MODEL_DIR}/n_queens.ozn
	
	