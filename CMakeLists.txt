cmake_minimum_required(VERSION 3.23 FATAL_ERROR)
set(CMAKE_POLICY_DEFAULT_CMP0169 OLD)
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PROJECT_NAME atlantis)
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
if(NOT CPM_SOURCE_CACHE AND NOT DEFINED ENV{CPM_SOURCE_CACHE})
  set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cpm-cache" CACHE PATH
      "Directory to share CPM dependency sources across build trees" FORCE)
endif()
if(CPM_SOURCE_CACHE AND NOT DEFINED ENV{CPM_SOURCE_CACHE})
  set(ENV{CPM_SOURCE_CACHE} "${CPM_SOURCE_CACHE}")
endif()
include(cmake/CPM.cmake)

set(ATLANTIS_SANITIZERS "" CACHE STRING
    "Comma- or semicolon-separated list of sanitizers to enable (address, undefined, thread)")

set(_atlantis_supported_sanitizers address undefined thread)
set(_atlantis_sanitizer_flags "")
if(ATLANTIS_SANITIZERS)
  if(MSVC)
    message(FATAL_ERROR "ATLANTIS_SANITIZERS is only supported with GCC- and Clang-like toolchains")
  endif()

  string(REPLACE "," ";" _atlantis_requested_sanitizers "${ATLANTIS_SANITIZERS}")
  separate_arguments(_atlantis_requested_sanitizers)
  list(REMOVE_DUPLICATES _atlantis_requested_sanitizers)

  foreach(_atlantis_sanitizer IN LISTS _atlantis_requested_sanitizers)
    if(NOT _atlantis_sanitizer IN_LIST _atlantis_supported_sanitizers)
      message(FATAL_ERROR
        "Unsupported sanitizer '${_atlantis_sanitizer}'. "
        "Use one or more of: ${_atlantis_supported_sanitizers}")
    endif()
    list(APPEND _atlantis_sanitizer_flags "-fsanitize=${_atlantis_sanitizer}")
  endforeach()

  if("thread" IN_LIST _atlantis_requested_sanitizers
     AND ("address" IN_LIST _atlantis_requested_sanitizers
          OR "undefined" IN_LIST _atlantis_requested_sanitizers))
    message(FATAL_ERROR "ThreadSanitizer must be used without address/undefined sanitizers")
  endif()
endif()

CPMAddPackage(
  NAME PackageProject.cmake
  VERSION 1.13.0
  GITHUB_REPOSITORY "TheLartians/PackageProject.cmake"
  GIT_TAG "v1.13.0"
)

# Apply warning flags only to C++ compilation. The C++ standard itself is
# configured via CMAKE_CXX_STANDARD above.
if(MSVC)
  add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:/permissive>
    $<$<COMPILE_LANGUAGE:CXX>:/W4>
    $<$<COMPILE_LANGUAGE:CXX>:/w14640>
  )
elseif(APPLE)
  add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>
    $<$<COMPILE_LANGUAGE:CXX>:-pedantic>
  )
elseif(NOT ATLANTIS_SANITIZERS)
  add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>
    $<$<COMPILE_LANGUAGE:CXX>:-pedantic>
    $<$<COMPILE_LANGUAGE:CXX>:-static>
  )
else()
  add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>
    $<$<COMPILE_LANGUAGE:CXX>:-pedantic>
  )
endif()

if(CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  # If we are debugging then add debugging flag
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g2 -O0")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g0 -O3 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
endif()

if(_atlantis_sanitizer_flags)
  add_compile_options(${_atlantis_sanitizer_flags} -fno-omit-frame-pointer)
  add_link_options(${_atlantis_sanitizer_flags})
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

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
if(EXISTS "${CPM_SOURCE_CACHE}/gecode")
  set(GECODE_INCLUDE "${CPM_SOURCE_CACHE}/gecode")
endif()
find_package(Gecode 6.0 REQUIRED COMPONENTS Driver Int Kernel Search)

if(NOT GECODE_LIBRARIES)
  message("Gecode not found, installing.")
  set(_atlantis_gecode_source_dir "${CPM_SOURCE_CACHE}/gecode-6.2.0")
  file(MAKE_DIRECTORY "${_atlantis_gecode_source_dir}/build")

  set(GECODE_INCLUDE "${CPM_SOURCE_CACHE}/gecode")
  file(MAKE_DIRECTORY "${GECODE_INCLUDE}")

  execute_process(COMMAND ${CMAKE_COMMAND} -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ${CMAKE_CXX_FLAGS} -S ${_atlantis_gecode_source_dir} -B ${_atlantis_gecode_source_dir}/build
          WORKING_DIRECTORY "${_atlantis_gecode_source_dir}"
          RESULT_VARIABLE _atlantis_gecode_generate_result)
  if(NOT _atlantis_gecode_generate_result EQUAL 0)
    message(FATAL_ERROR "Failed to generate Gecode cmake: ${_atlantis_gecode_generate_result}")
  endif()
  include(ProcessorCount)
  ProcessorCount(NUM_PROCESSORS)
  if(NUM_PROCESSORS LESS 0)
    set(NUM_PROCESSORS 1)
    elif(NUM_PROCESSORS GREATER 16)
    set(NUM_PROCESSORS 16)
  endif()

  execute_process(COMMAND ${CMAKE_COMMAND} --build   "${_atlantis_gecode_source_dir}/build" --target fzn-gecode -j ${NUM_PROCESSORS}
          WORKING_DIRECTORY "${_atlantis_gecode_source_dir}"
          RESULT_VARIABLE _atlantis_gecode_build_result)
  if(NOT _atlantis_gecode_build_result EQUAL 0)
    message(FATAL_ERROR "Failed to build Gecode: ${_atlantis_gecode_build_result}")
  endif()

  set(_atlantis_gecode_install_result 0)
  foreach(NUM_TRIES RANGE 1 5)
    execute_process(COMMAND ${CMAKE_COMMAND} --install "${_atlantis_gecode_source_dir}/build" --prefix "${GECODE_INCLUDE}"
            WORKING_DIRECTORY "${_atlantis_gecode_source_dir}"
            OUTPUT_QUIET
            RESULT_VARIABLE _atlantis_gecode_install_result)
    if(NOT _atlantis_gecode_install_result EQUAL 0)
      message(WARNING "Failed to install Gecode: ${_atlantis_gecode_install_result}, attempt ${NUM_TRIES}.")
    else()
      break()
    endif()
  endforeach()
  if(NOT _atlantis_gecode_install_result EQUAL 0)
    message(FATAL_ERROR "Failed to install Gecode: ${_atlantis_gecode_install_result}")
  else()
    message("Successfully installed Gecode")
  endif()
  find_package(Gecode 6.2 REQUIRED COMPONENTS Driver Int Kernel Search)
endif()

# Link Gecode to target test
message("GECODE_FOUND: ${GECODE_FOUND}")
message("GECODE_INCLUDE_DIRS: ${GECODE_INCLUDE_DIRS}")
message("GECODE_LIBRARIES: ${GECODE_LIBRARIES}")
message("GECODE_TARGETS: ${GECODE_TARGETS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}-Wl,--copy-dt-needed-entries")
target_link_libraries(${PROJECT_LIB} Gecode::Driver Gecode::Int Gecode::Kernel Gecode::Search)

include(FetchContent)
FetchContent_Declare(
  fznparser
  GIT_REPOSITORY https://github.com/astra-uu-se/fznparser
  GIT_TAG v1.2.3
)
FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts
  GIT_TAG v3.3.1
)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG 11.2.0
)

FetchContent_MakeAvailable(cxxopts fmt fznparser)

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
elseif(NOT ATLANTIS_SANITIZERS)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT lto_supported OUTPUT error)

  if(lto_supported)
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET ${PROJECT_EXEC} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(STATUS "IPO / LTO not supported: <${error}>")
  endif()
elseif(ATLANTIS_SANITIZERS)
  message(STATUS "IPO / LTO disabled because ATLANTIS_SANITIZERS=${ATLANTIS_SANITIZERS}")
endif()

# Options. Turn on with 'cmake -Dmyvarname=ON'.
option(BUILD_TESTS "Build all tests." OFF) # Makes boolean 'test' available.
option(BUILD_BENCHMARKS "Build all benchmarks." OFF) # Makes boolean 'benchmark' available.
option(MORE_STATS "Track more statistics in benchmarks" OFF)

if (MORE_STATS)
  add_compile_definitions(MORE_STATS)
endif()

if (BUILD_TESTS)
  include(CTest)
  enable_testing()
endif()

if(BUILD_TESTS OR BUILD_BENCHMARKS)
  add_compile_definitions(FZN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fzn-models")

  # Google Test is required for testing and benchmarking
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
  )

  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

if(BUILD_TESTS)
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
    GIT_REPOSITORY https://github.com/astra-uu-se/rapidcheck
    GIT_TAG g++-23
  )

  FetchContent_MakeAvailable(googletest rapidcheck)

  if(APPLE AND TARGET rapidcheck)
    # rapidcheck's g++-23 branch relies on transitive standard-library includes
    # that libc++ does not provide. Force-include a tiny compatibility shim
    # only for the rapidcheck target on Apple platforms.
    target_compile_options(
      rapidcheck PUBLIC
      -include ${CMAKE_CURRENT_SOURCE_DIR}/cmake/rapidcheck_apple_compat.hpp
    )
  endif()

  # #############
  # Unit Tests
  # #############
  file(GLOB_RECURSE TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp ${PROJECT_SOURCE_DIR}/test/*.h ${PROJECT_SOURCE_DIR}/test/*.hpp)
  add_executable(runUnitTests ${TEST_SRC_FILES})

  # Standard linking to gtest, gmock, and rapidcheck.
  target_link_libraries(
    runUnitTests
    gmock_main
    rapidcheck_gtest
    Threads::Threads
    -lm
    ${PROJECT_LIB}
  )

  # This is so you can do 'make test' to see all your tests run, instead of
  # manually running the executable runUnitTests to see those specific tests.
  # add_test(NAME "UnitTests" COMMAND "runUnitTests")
  set_target_properties(runUnitTests PROPERTIES FOLDER tests)

  include(GoogleTest)

  gtest_discover_tests(runUnitTests)
endif()

if(BUILD_BENCHMARKS)
  # ###############################
  # Benchmarks
  # ###############################
  set(BENCHMARK_ENABLE_TESTING off) # to suppress benchmark internal tests
  FetchContent_Declare(googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.4)

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
  DEPENDENCIES "fmt 8.1.1;fznparser v1.2.3"
)
