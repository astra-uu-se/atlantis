C_COMPILER=$(shell which gcc-11)
CXX_COMPILER=$(shell which g++-11)
export CMAKE_OPTIONS+= ${ENV_CMAKE_OPTIONS} -DCMAKE_C_COMPILER=${C_COMPILER} -DCMAKE_CXX_COMPILER=${CXX_COMPILER}
MKFILE_PATH=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR=${MKFILE_PATH}build

CMAKE=$(shell which cmake)

BENCHMARK_JSON_DIR=${MKFILE_PATH}benchmark-json
NUM_BENCHMARK_REPETITIONS=5
BENCHMARK_FILTER="^(GolombRuler|ExtremeDynamic|ExtremeStatic|MagicSquare|NQueens|TSPTW|VesselLoading)"
BENCHMARK_PLOT_DIR=${MKFILE_PATH}plots

MZN_MODEL_DIR=${MKFILE_PATH}mzn-models
FZN_MODEL_DIR=${MKFILE_PATH}fzn-models
MZN_PATH=~/minizinc/MiniZincIDE-build1187512964-bundle-linux-x86_64/bin/minizinc
ifeq ($(shell test -e ${MZN_PATH} && echo yes),yes)
	MZN=$(shell realpath ${MZN_PATH})
else
	MZN=$(shell which minizinc)
endif

export MZN_SOLVER_PATH=${BUILD_DIR}

.PHONY: clean
clean:
	rm -rf ${BUILD_DIR}

.PHONY: build
build:
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR}; $(CMAKE) ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=Release \
	                                           -DBUILD_TESTS:BOOL=OFF \
														                 -DBUILD_BENCHMARKS:BOOL=OFF ..
	cd ${BUILD_DIR}; $(MAKE)

.PHONY: build-tests
build-tests:
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR}; $(CMAKE) ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=Debug \
	                                           -DBUILD_TESTS:BOOL=ON \
	                                           -DBUILD_BENCHMARKS:BOOL=OFF ..
	cd ${BUILD_DIR}; $(MAKE)

.PHONY: build-benchmarks
build-benchmarks:
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR}; $(CMAKE) ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=Release \
	                                           -DBUILD_TESTS:BOOL=OFF \
	                                           -DBUILD_BENCHMARKS:BOOL=ON ..; \
	cd ${BUILD_DIR}; $(MAKE)

.PHONY: run
run: build
	exec ${BUILD_DIR}/atlantis

.PHONY: run-tests
run-tests: build-tests
	exec ${BUILD_DIR}/runUnitTests

.PHONY: run-benchmarks
run-benchmarks: build-benchmarks
	exec ${BUILD_DIR}/runBenchmarks

.PHONY: benchmark
benchmark: build-benchmarks
	mkdir -p ${BENCHMARK_JSON_DIR}
	mkdir -p ${BENCHMARK_PLOT_DIR}
	$(eval $@_TIMESTAMP := $(shell date +"%Y-%m-%d-%H-%M-%S-%3N"))
	$(eval $@_JSON_FILE := ${BENCHMARK_JSON_DIR}/${$@_TIMESTAMP}.json)
	exec ${BUILD_DIR}/runBenchmarks --benchmark_format=json \
	                                --benchmark_out=${$@_JSON_FILE} \
									--benchmark_repetitions=${NUM_BENCHMARK_REPETITIONS} \
									--benchmark_filter=${BENCHMARK_FILTER}
	python3 ${MKFILE_PATH}plot-formatter.py -v --input=${$@_JSON_FILE} --file-suffix=${$@_TIMESTAMP} --output-dir=${BENCHMARK_PLOT_DIR}

.PHONY: all
all: clean build build-tests build-benchmarks

.PHONY: fzn
fzn:
	$(MZN) --solver ${MZN_SOLVER_PATH}/atlantis.msc -c \
		${MZN_MODEL_DIR}/car_sequencing.mzn \
		${MZN_MODEL_DIR}/car_sequencing.dzn \
		--fzn ${FZN_MODEL_DIR}/car_sequencing.fzn \
		--no-output-ozn
	$(MZN) --solver ${MZN_SOLVER_PATH}/atlantis.msc -c \
		${MZN_MODEL_DIR}/magic_square.mzn \
		-D n=3 \
		--fzn ${FZN_MODEL_DIR}/magic_square.fzn \
		--no-output-ozn
	$(MZN) --solver ${MZN_SOLVER_PATH}/atlantis.msc -c \
		${MZN_MODEL_DIR}/tsp_alldiff.mzn \
		${MZN_MODEL_DIR}/tsp_17.dzn \
		--fzn ${FZN_MODEL_DIR}/tsp_alldiff.fzn \
		--no-output-ozn
	$(MZN) --solver ${MZN_SOLVER_PATH}/atlantis.msc -c \
		${MZN_MODEL_DIR}/n_queens.mzn \
		-D n=32 \
		--fzn ${FZN_MODEL_DIR}/n_queens.fzn \
		--no-output-ozn
	$(MZN) --solver ${MZN_SOLVER_PATH}/atlantis.msc -c \
		${MZN_MODEL_DIR}/comp_domain_ann.mzn \
		--fzn ${FZN_MODEL_DIR}/comp_domain_ann.fzn \
		--no-output-ozn

.PHONY: clang-format
clang-format:
	find ${MKFILE_PATH}/benchmark/ ${MKFILE_PATH}/include/ ${MKFILE_PATH}/src/ ${MKFILE_PATH}/test/ \
	  -iname *.[ch]pp | xargs clang-format -i -style=Google
