# Taken from
# https://github.com/gujans/travis-gtest-cmake-example/blob/master/CMakeLists.txt

cmake_minimum_required(VERSION 3.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PROJECT_NAME cbls)
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Check if microsoft visual code. I have no idea if this works
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /permissive /W4 /w14640 ")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic")
endif()

if(CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  # If we are debugging then add debugging flag
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g2 -O0") 
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g0 -O3 -DNDEBUG ")
endif()

message("${CMAKE_BUILD_TYPE}")

#----------------------------------------------------------------------------
# Define project sources and includes
#----------------------------------------------------------------------------

set(COMMON_INCLUDES ${PROJECT_SOURCE_DIR}/include)
set(GOOGLETEST_PATH ${PROJECT_SOURCE_DIR}/ext/googletest)
# Set the 'library' name. The library consists of all cpp, hpp, and h files except
# main in ./src
set(PROJECT_LIB ${PROJECT_NAME}_lib)

# includes (hpp/h) files are in ./include
include_directories(${COMMON_INCLUDES})

# Retrieve all cpp, hpp, and h files in ./src
file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/include/*.h ${PROJECT_SOURCE_DIR}/include/*.hpp)
# remove main from the src files
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/main.cpp)
# remove fznparser files from the src files
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/fznparser/src/main.cpp)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/fznparser/src/FznVisitor.cpp)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/fznparser/src/structure.cpp)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/fznparser/src/variable.cpp)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/fznparser/src/statistics.cpp)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/fznparser/src/constraintbox.cpp)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/fznparser/src/constraint.cpp)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/fznparser/src/maps.cpp)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/fznparser/src/model.cpp)


################################
# Normal Libraries & Executables
################################
# The src files make up the library
add_library(${PROJECT_LIB} ${SRC_FILES})
# The executable is the main file
add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/src/main.cpp)

# Link the src library with the project
target_link_libraries(${PROJECT_NAME} ${PROJECT_LIB})

################################
# Testing
################################

# Options. Turn on with 'cmake -Dmyvarname=ON'.
option(BUILD_TESTS "Build all tests." OFF) # Makes boolean 'test' available.
option(BUILD_BENCHMARKS "Build all benchmarks." OFF) # Makes boolean 'benchmark' available.
option(BENCHMARK_ENABLE_TESTING "Build GTEST for GBENCHMARK" OFF)

if (BUILD_TESTS)
  # This adds another subdirectory, which has 'project(gtest)'.
  add_subdirectory(ext/googletest)
  add_subdirectory(ext/rapidcheck)
  add_subdirectory(ext/rapidcheck/extras/gtest)
  add_subdirectory(ext/rapidcheck/extras/gmock)

  include(CTest)

  enable_testing()

  include(GoogleTest)

  set_target_properties(gtest PROPERTIES FOLDER ext)
  set_target_properties(gtest_main PROPERTIES FOLDER ext)
  set_target_properties(gmock PROPERTIES FOLDER ext)
  set_target_properties(gmock_main PROPERTIES FOLDER ext)

  # Include the gtest library. gtest_SOURCE_DIR is available due to
  # 'project(gtest)' above.
  include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

  # Include the gmock library. gmock_SOURCE_DIR is available due to
  # 'project(gmock)' above.
  include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

  ##############
  # Unit Tests
  ##############
  file(GLOB_RECURSE TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp ${PROJECT_SOURCE_DIR}/test/*.h ${PROJECT_SOURCE_DIR}/test/*.hpp)
  add_executable(runUnitTests ${TEST_SRC_FILES})

  # Standard linking to gtest stuff.
  target_link_libraries(runUnitTests gtest gtest_main)

  target_link_libraries(runUnitTests gmock gmock_main)
  
  target_link_libraries(runUnitTests rapidcheck rapidcheck_gtest rapidcheck_gmock)

  # Extra linking for the project.
  target_link_libraries(runUnitTests ${PROJECT_LIB})

  # This is so you can do 'make test' to see all your tests run, instead of
  # manually running the executable runUnitTests to see those specific tests.
  # add_test(NAME "UnitTests" COMMAND "runUnitTests")

  gtest_discover_tests(runUnitTests
        # set a working directory so your project root so that you can find test data via paths relative to the project root
        WORKING_DIRECTORY ${PROJECT_DIR}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
    )
  set_target_properties(runUnitTests PROPERTIES FOLDER tests)

endif()
if (BUILD_BENCHMARKS)
  # This adds another subdirectory, which has 'project(gtest)'.
  add_subdirectory(ext/benchmark)

  ##############
  # Benchmarks
  ##############
  file(GLOB_RECURSE BENCHMARK_SRC_FILES ${PROJECT_SOURCE_DIR}/benchmark/*.cpp ${PROJECT_SOURCE_DIR}/benchmark/*.h ${PROJECT_SOURCE_DIR}/benchmark/*.hpp)
  add_executable(runBenchmarks ${BENCHMARK_SRC_FILES})

  # Link to benchmark
  target_link_libraries(runBenchmarks benchmark::benchmark)

  # Link to project wide benchmark main.
  target_link_libraries(runBenchmarks benchmark::benchmark_main)

  # Link to cbls_lib
  target_link_libraries(runBenchmarks ${PROJECT_LIB})
endif()
