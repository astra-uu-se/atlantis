C_COMPILER=$(shell which gcc-13)
CXX_COMPILER=$(shell which g++-13)
export CMAKE_OPTIONS+= ${ENV_CMAKE_OPTIONS} -DCMAKE_C_COMPILER=${C_COMPILER} -DCMAKE_CXX_COMPILER=${CXX_COMPILER}
MKFILE_PATH=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR=${MKFILE_PATH}build

CMAKE=$(shell which cmake)

BENCHMARK_JSON_DIR=${MKFILE_PATH}benchmark-json
NUM_BENCHMARK_REPETITIONS=5
BENCHMARK_FILTER="^(ExtremeDynamic|ExtremeStatic|GolombRuler|MagicSquare|NQueens|TSPTW|VesselLoading)\/[A-Za-z]"
BENCHMARK_FILTER_SYNTH="^(ElementVarTree|LinearTree|TSP|TSPTWAllDiff)\/[A-Za-z]"
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

define compile_mzn
	$(MZN) --solver ${MZN_SOLVER_PATH}/atlantis.msc -c \
		${MZN_MODEL_DIR}/$(1).mzn \
		--fzn ${FZN_MODEL_DIR}/$(1).fzn \
		--no-output-ozn
	$(MZN) --solver ${MZN_SOLVER_PATH}/atlantis.msc -c --use-gecode \
		${MZN_MODEL_DIR}/$(1).mzn \
		--fzn ${FZN_MODEL_DIR}/$(1)_gecode.fzn \
		--no-output-ozn
endef

define compile_mzn_dzn
	$(MZN) --solver ${MZN_SOLVER_PATH}/atlantis.msc -c \
		${MZN_MODEL_DIR}/$(1).mzn \
		${MZN_MODEL_DIR}/$(2).dzn \
		--fzn ${FZN_MODEL_DIR}/$(1).fzn \
		--no-output-ozn
	$(MZN) --solver ${MZN_SOLVER_PATH}/atlantis.msc -c --use-gecode \
		${MZN_MODEL_DIR}/$(1).mzn \
		${MZN_MODEL_DIR}/$(2).dzn \
		--fzn ${FZN_MODEL_DIR}/$(1)_gecode.fzn \
		--no-output-ozn
endef

define compile_mzn_param
	$(MZN) --solver ${MZN_SOLVER_PATH}/atlantis.msc -c \
		${MZN_MODEL_DIR}/$(1).mzn \
		-D $(2) \
		--fzn ${FZN_MODEL_DIR}/$(1).fzn \
		--no-output-ozn
	$(MZN) --solver ${MZN_SOLVER_PATH}/atlantis.msc -c --use-gecode \
		${MZN_MODEL_DIR}/$(1).mzn \
		-D $(2) \
		--fzn ${FZN_MODEL_DIR}/$(1)_gecode.fzn \
		--no-output-ozn
endef

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

.PHONY: build-benchmarks-debug
build-benchmarks-debug:
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR}; $(CMAKE) ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=Debug \
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

.PHONY: benchmark-synth
benchmark-synth: build-benchmarks
	mkdir -p ${BENCHMARK_JSON_DIR}
	mkdir -p ${BENCHMARK_PLOT_DIR}
	$(eval $@_TIMESTAMP := $(shell date +"%Y-%m-%d-%H-%M-%S-%3N"))
	$(eval $@_JSON_FILE := ${BENCHMARK_JSON_DIR}/${$@_TIMESTAMP}.json)
	exec ${BUILD_DIR}/runBenchmarks --benchmark_format=json \
	                                --benchmark_out=${$@_JSON_FILE} \
									--benchmark_repetitions=${NUM_BENCHMARK_REPETITIONS} \
									--benchmark_filter=${BENCHMARK_FILTER_SYNTH}
	python3 ${MKFILE_PATH}plot-formatter.py -v --input=${$@_JSON_FILE} --file-suffix=${$@_TIMESTAMP} --output-dir=${BENCHMARK_PLOT_DIR}

.PHONY: all
all: clean build build-tests build-benchmarks

.PHONY: fzn
fzn:
	@$(call compile_mzn,comp_domain_ann)
	@$(call compile_mzn,simple_minimize)
	@$(call compile_mzn,all_different_minimize)
	@$(call compile_mzn_dzn,car_sequencing,car_sequencing)
	@$(call compile_mzn_dzn,tsp_alldiff,tsp_17)
	@$(call compile_mzn_dzn,tsp,tsp_17)
	@$(call compile_mzn_param,magic_square,n=3)
	@$(call compile_mzn_param,n_queens,n=16)

.PHONY: clang-format
clang-format:
	find ${MKFILE_PATH}/benchmark/ ${MKFILE_PATH}/include/ ${MKFILE_PATH}/src/ ${MKFILE_PATH}/test/ \
	  -iname *.[ch]pp | xargs clang-format -i -style=Google
