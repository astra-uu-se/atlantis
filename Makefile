export CMAKE_OPTIONS+= -DBUILD_TESTS:BOOL=ON ${ENV_CMAKE_OPTIONS}
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

submodule:
	git submodule init
	git submodule update

test: build
	cd ${BUILD_DIR}; \
	ctest

run: build
	exec ./${BUILD_DIR}/cbls