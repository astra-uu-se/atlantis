export CMAKE_OPTIONS+= ${ENV_CMAKE_OPTIONS}

CMAKE=/usr/bin/cmake
MKFILE_PATH := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR=${MKFILE_PATH}build

all: clean build

clean:
	rm -r ${BUILD_DIR}

build:
	mkdir -p ${BUILD_DIR}; \
	cd ${BUILD_DIR}; \
	$(CMAKE) ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=${CXX_FLAGS} ..; \
	$(MAKE)

tests:
	mkdir -p ${BUILD_DIR}; \
	cd ${BUILD_DIR}; \
	$(CMAKE) ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS:BOOL=ON -DCMAKE_CXX_FLAGS=${CXX_FLAGS} ..; \
	$(MAKE)

benchmarks:
	mkdir -p ${BUILD_DIR}; \
	cd ${BUILD_DIR}; \
	$(CMAKE) ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS:BOOL=ON -DCMAKE_CXX_FLAGS=${CXX_FLAGS} ..; \
	$(MAKE)

submodule-update:
	git submodule init
	git submodule update

submodule-update-remote:
	git submodule init
	git submodule update --remote --merge

submodule-googletest:
	mkdir -p ${MKFILE_PATH}ext/googletest/build
	cd ${MKFILE_PATH}ext/googletest/build; \
	$(CMAKE) -DCMAKE_BUILD_TYPE=Release ..; \
	sudo $(MAKE) install;

submodule-benchmark:
	mkdir -p ${MKFILE_PATH}ext/benchmark/build
	cd ${MKFILE_PATH}ext/benchmark/build; \
	$(CMAKE) -DCMAKE_BUILD_TYPE=Release -DGOOGLETEST_PATH:STRING=${MKFILE_PATH}ext/googletest ..; \
	sudo $(MAKE) install;

submodule-install: submodule-update submodule-googletest submodule-benchmark

test: build
	cd ${BUILD_DIR}; \
	ctest -j6 -C Debug -T test --output-on-failure

run: build
	exec ${BUILD_DIR}/cbls