# CMake generated Testfile for 
# Source directory: /home/frekn832/cbls
# Build directory: /home/frekn832/cbls/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(UnitTests "/home/frekn832/cbls/build/runUnitTests")
subdirs("ext/googletest")
