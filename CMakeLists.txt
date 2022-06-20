cmake_minimum_required(VERSION 3.20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PROJECT_NAME cbls)
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Check if microsoft visual code. I have no idea if this works
if (MSVC)
    add_compile_options(/std:c++20 /permissive /W4 /w14640)
else ()
    add_compile_options(-std=c++20 -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic)
endif ()

if (CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    # If we are debugging then add debugging flag
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g2 -O0")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g0 -O3 -DNDEBUG ")
endif ()

#----------------------------------------------------------------------------
# Define project sources and includes
#----------------------------------------------------------------------------

set(COMMON_INCLUDES ${PROJECT_SOURCE_DIR}/include)
# Set the 'library' name. The library consists of all cpp, hpp, and h files except
# main in ./src
set(PROJECT_LIB ${PROJECT_NAME}_lib)

# Retrieve all cpp, hpp, and h files in ./src
file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/include/*.h ${PROJECT_SOURCE_DIR}/include/*.hpp)
# remove main from the src files
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/main.cpp)


################################
# Normal Libraries & Executables
################################
# The src files make up the library
add_library(${PROJECT_LIB} ${SRC_FILES})
# The executable is the main file
add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/src/main.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
        fznparser
        GIT_REPOSITORY git@github.com:astra-uu-se/fznparser.git
        GIT_TAG a4bfe45eec246b54b44616347061d58fb02355a9
)
FetchContent_Declare(
        cxxopts
        GIT_REPOSITORY https://github.com/jarro2783/cxxopts
        GIT_TAG v3.0.0
)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG 8.1.1
)

FetchContent_MakeAvailable(cxxopts fznparser fmt)

# includes (hpp/h) files are in ./include
target_include_directories(${PROJECT_LIB}
        PRIVATE
        ${COMMON_INCLUDES}
        ${PROJECT_SOURCE_DIR}/ext
        PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${COMMON_INCLUDES}>)


target_link_libraries(${PROJECT_LIB} PUBLIC fznparser::fznparser PRIVATE fmt::fmt)

# Link the src library with the project
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_LIB} cxxopts::cxxopts fmt::fmt)

if (CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
else ()
    include(CheckIPOSupported)
    check_ipo_supported(RESULT lto_supported OUTPUT error)

    if (lto_supported)
        message(STATUS "IPO / LTO enabled")
        set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else ()
        message(STATUS "IPO / LTO not supported: <${error}>")
    endif ()
endif ()

# Options. Turn on with 'cmake -Dmyvarname=ON'.
option(BUILD_TESTS "Build all tests." OFF) # Makes boolean 'test' available.
option(BUILD_BENCHMARKS "Build all benchmarks." OFF) # Makes boolean 'benchmark' available.

if (BUILD_TESTS OR BUILD_BENCHMARKS)
    # Google Test is required for testing and benchmarking
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.11.0
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif ()

if (BUILD_TESTS)
    ################################
    # Testing
    ################################

    # Required for rapidcheck
    set(RC_ENABLE_GTEST ON CACHE BOOL "Rapidcheck GTest Support" FORCE)
    set(RC_ENABLE_GMOCK ON CACHE BOOL "Rapidcheck GMock Support" FORCE)
    set(RC_ENABLE_BOOST ON CACHE BOOL "Rapidcheck Boost Support" FORCE)
    set(RC_ENABLE_RTTI ON CACHE BOOL "Rapidcheck RTTI Support" FORCE)

    FetchContent_Declare(rapidcheck
            GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
            GIT_TAG 33d15a858e3125f5af61a655f390f1cbc2f272ba
            )

    FetchContent_MakeAvailable(googletest rapidcheck)

    include(CTest)

    enable_testing()

    ##############
    # Unit Tests
    ##############
    file(GLOB_RECURSE TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp ${PROJECT_SOURCE_DIR}/test/*.h ${PROJECT_SOURCE_DIR}/test/*.hpp)
    add_executable(runUnitTests ${TEST_SRC_FILES})

    # Standard linking to gtest, gmock, and rapidcheck.
    target_link_libraries(
            runUnitTests
            gtest
            gtest_main
            gmock
            gmock_main
            rapidcheck_gtest
            rapidcheck_gmock
            Threads::Threads
            -lm
            ${PROJECT_LIB}
            fmt::fmt
    )

    # This is so you can do 'make test' to see all your tests run, instead of
    # manually running the executable runUnitTests to see those specific tests.
    # add_test(NAME "UnitTests" COMMAND "runUnitTests")

    set_target_properties(runUnitTests PROPERTIES FOLDER tests)

endif ()
if (BUILD_BENCHMARKS)
    ################################
    # Benchmarks
    ################################
    FetchContent_Declare(googlebenchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG e451e50e9b8af453f076dec10bd6890847f1624e)

    FetchContent_MakeAvailable(googletest googlebenchmark)

    file(GLOB_RECURSE BENCHMARK_SRC_FILES ${PROJECT_SOURCE_DIR}/benchmark/*.cpp ${PROJECT_SOURCE_DIR}/benchmark/*.h ${PROJECT_SOURCE_DIR}/benchmark/*.hpp)

    add_executable(runBenchmarks ${BENCHMARK_SRC_FILES})

    # Link to benchmark
    target_link_libraries(
            runBenchmarks
            benchmark::benchmark
            benchmark::benchmark_main
            Threads::Threads
            -lm
            ${PROJECT_LIB}
    )
endif ()