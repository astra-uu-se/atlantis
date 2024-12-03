cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PROJECT_NAME atlantis)
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(GNUInstallDirs)
include(cmake/CPM.cmake)

CPMAddPackage(
  NAME PackageProject.cmake
  VERSION 1.10.0
  GITHUB_REPOSITORY "TheLartians/PackageProject.cmake"
  GIT_TAG "v1.10.0"
)

# Check if microsoft visual code. I have no idea if this works
if(MSVC)
  add_compile_options(/std:c++20 /permissive /W4 /w14640)
else()
  add_compile_options(-std=c++20 -Wall -Wextra -Wnon-virtual-dtor -pedantic -static)
endif()

if(CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  # If we are debugging then add debugging flag
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g2 -O0")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g0 -O3 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
endif()

# ----------------------------------------------------------------------------
# Define project sources and includes
# ----------------------------------------------------------------------------
set(COMMON_INCLUDES ${PROJECT_SOURCE_DIR}/include)

# Set the 'library' name. The library consists of all cpp, hpp, and h files except
# main in ./src
set(PROJECT_LIB ${PROJECT_NAME})

# Set the 'executable' name. The executable consists of the library together with
# main.cpp in ./src
set(PROJECT_EXEC fzn-${PROJECT_NAME})

# Retrieve all cpp, hpp, and h files in ./src
file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/include/*.h ${PROJECT_SOURCE_DIR}/include/*.hpp)

# remove main from the src files
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/main.cpp)

# ###############################
# Normal Libraries & Executables
# ###############################
# The src files make up the library
add_library(${PROJECT_LIB} ${SRC_FILES})

# The executable is the main file
add_executable(${PROJECT_EXEC} ${PROJECT_SOURCE_DIR}/src/main.cpp)

# Add executable to --target install
install(
  TARGETS ${PROJECT_EXEC}
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Add minizinc library to --target install
install(
  DIRECTORY share/minizinc/atlantis
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/minizinc/
)

# Create a `atlantis.msc` in the build directory (for each config)
set(MZN_LOC ${CMAKE_SOURCE_DIR}/share/minizinc/atlantis)

if(CMAKE_CONFIGURATION_TYPES)
  foreach(CNF IN LISTS CMAKE_CONFIGURATION_TYPES)
    set(EXEC_LOC ${CMAKE_BINARY_DIR}/${CNF}/${PROJECT_EXEC})
    string(TOLOWER "${CNF}" CNFL)
    configure_file(share/minizinc/solvers/atlantis.msc.in atlantis-${CNFL}.msc)
  endforeach()
else()
  set(EXEC_LOC ${CMAKE_BINARY_DIR}/${PROJECT_EXEC})
  configure_file(share/minizinc/solvers/atlantis.msc.in atlantis.msc)
endif()

# Create a `atlantis.msc` that is added to --target install
set(MZN_LOC ../atlantis)
file(RELATIVE_PATH EXEC_LOC ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/minizinc/solvers ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${PROJECT_EXEC})
configure_file(share/minizinc/solvers/atlantis.msc.in share/minizinc/solvers/atlantis.msc)
install(
  FILES ${CMAKE_BINARY_DIR}/share/minizinc/solvers/atlantis.msc
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/minizinc/solvers/
)
unset(EXEC_LOC)
unset(MZN_LOC)

option(FMT_INSTALL "Enable install for fmt project" ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
  fznparser
  GIT_REPOSITORY https://github.com/astra-uu-se/fznparser
  GIT_TAG v1.0.3
)
FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts
  GIT_TAG v3.2.0
)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG 11.0.2
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

target_link_libraries(${PROJECT_LIB} fznparser::fznparser fmt::fmt)

# Link the src library with the project
target_link_libraries(${PROJECT_EXEC} ${PROJECT_LIB} cxxopts::cxxopts fmt::fmt)

if(CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
else()
  include(CheckIPOSupported)
  check_ipo_supported(RESULT lto_supported OUTPUT error)

  if(lto_supported)
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET ${PROJECT_EXEC} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(STATUS "IPO / LTO not supported: <${error}>")
  endif()
endif()

# Options. Turn on with 'cmake -Dmyvarname=ON'.
option(BUILD_TESTS "Build all tests." OFF) # Makes boolean 'test' available.
option(BUILD_BENCHMARKS "Build all benchmarks." OFF) # Makes boolean 'benchmark' available.

if(BUILD_TESTS OR BUILD_BENCHMARKS)
  # Google Test is required for testing and benchmarking
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
  )

  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

if(BUILD_TESTS)
  add_compile_definitions(FZN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fzn-models")
  add_compile_definitions(FZN_CHALLENGE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fzn-challanges")

  # ###############################
  # Testing
  # ###############################

  # Required for rapidcheck
  set(RC_ENABLE_GTEST ON CACHE BOOL "Rapidcheck GTest Support" FORCE)
  set(RC_ENABLE_GMOCK ON CACHE BOOL "Rapidcheck GMock Support" FORCE)
  set(RC_ENABLE_BOOST ON CACHE BOOL "Rapidcheck Boost Support" FORCE)
  set(RC_ENABLE_RTTI ON CACHE BOOL "Rapidcheck RTTI Support" FORCE)

  FetchContent_Declare(rapidcheck
    GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
    GIT_TAG ff6af6fc683159deb51c543b065eba14dfcf329b
  )

  FetchContent_MakeAvailable(googletest rapidcheck)

  include(CTest)

  enable_testing()

  # #############
  # Unit Tests
  # #############
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
  )

  # This is so you can do 'make test' to see all your tests run, instead of
  # manually running the executable runUnitTests to see those specific tests.
  # add_test(NAME "UnitTests" COMMAND "runUnitTests")
  set_target_properties(runUnitTests PROPERTIES FOLDER tests)
endif()

if(BUILD_BENCHMARKS)
  # ###############################
  # Benchmarks
  # ###############################
  set(BENCHMARK_ENABLE_TESTING off) # to suppress benchmark internal tests
  FetchContent_Declare(googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.1)

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
endif()

add_executable(queens EXCLUDE_FROM_ALL ${PROJECT_SOURCE_DIR}/examples/queens.cpp)
target_link_libraries(queens ${PROJECT_LIB})

# ---- Create an installable target ----
# this allows users to install and find the library via `find_package()`.

# the location where the project's version header will be placed should match
# the project's regular header paths
string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)

packageProject(
  NAME ${PROJECT_LIB}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/
  VERSION_HEADER "${VERSION_HEADER_LOCATION}"
  DEPENDENCIES "fmt 8.1.1;fznparser 0.2"
)
