export CMAKE_OPTIONS+= ${ENV_CMAKE_OPTIONS}
export BUILD_DIR=build

all:
	make clean
	make build

clean:
	rm -r ${BUILD_DIR}

build:
	mkdir -p ${BUILD_DIR}; \
	cd ${BUILD_DIR}; \
	cmake ${CMAKE_OPTIONS} -DCMAKE_CXX_FLAGS=${CXX_FLAGS} ..; \
	make

tests:
	mkdir -p ${BUILD_DIR}; \
	cd ${BUILD_DIR}; \
	cmake ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS:BOOL=ON -DCMAKE_CXX_FLAGS=${CXX_FLAGS} ..; \
	make

benchmarks:
	mkdir -p ${BUILD_DIR}; \
	cd ${BUILD_DIR}; \
	cmake ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS:BOOL=ON -DCMAKE_CXX_FLAGS=${CXX_FLAGS} ..; \
	make

submodule:
	git submodule init
	git submodule update

test: build
	cd ${BUILD_DIR}; \
	ctest -j6 -C Debug -T test --output-on-failure

run: build
	exec ./${BUILD_DIR}/cbls