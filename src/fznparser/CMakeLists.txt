set(PROJECT_NAME fznparser)
# Set the 'library' name. The library consists of all cpp, hpp, and h files except
# main in ./src
set(PROJECT_LIB ${PROJECT_NAME}_lib)
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# minimum required CMAKE version
CMAKE_MINIMUM_REQUIRED(VERSION 3.7 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# compiler must be 11 or 14
set(CMAKE_CXX_STANDARD 17)

# required if linking to static library
add_definitions(-DANTLR4CPP_STATIC)

# using /MD flag for antlr4_runtime (for Visual C++ compilers only)
set(ANTLR4_WITH_STATIC_CRT OFF)
# add external build for antlrcpp
include(ExternalAntlr4Cpp)
# add antrl4cpp artifacts to project environment
include_directories(${ANTLR4_INCLUDE_DIRS})

# set variable pointing to the antlr tool that supports C++
# this is not required if the jar file can be found under PATH environment
# set(ANTLR_EXECUTABLE /usr/local/lib/antlr-4.9.1-complete.jar)
set(ANTLR_EXECUTABLE ${PROJECT_SOURCE_DIR}/thirdparty/antlr/antlr-4.9.1-complete.jar)

# add macros to generate ANTLR Cpp code from grammar
find_package(ANTLR REQUIRED)

# Call macro to add lexer and grammar to your build dependencies.
antlr_target(Sample ${PROJECT_SOURCE_DIR}/src/FlatZinc.g4
  PACKAGE fznparser
  VISITOR)

# include generated files in project environment
include_directories(${ANTLR_Sample_OUTPUT_DIR})

# Retrieve all cpp, hpp, and h files in ./src
file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp  ${PROJECT_SOURCE_DIR}/src/*.hpp)
# remove main from the src files
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/main.cpp)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/FznVisitor.cpp)

# The src files make up the library
add_library(${PROJECT_LIB} ${SRC_FILES})

# add generated grammar to demo binary target
add_executable(${PROJECT_NAME} src/main.cpp
  src/FznVisitor.cpp
               ${ANTLR_Sample_CXX_OUTPUTS})
target_link_libraries(${PROJECT_NAME} ${PROJECT_LIB} antlr4_static)
