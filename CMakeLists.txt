cmake_minimum_required(VERSION 2.8.8)
project(llvmtcl)

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_package(TCL)
find_package(TclSh)
find_package(TclStub)

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

set (PACKAGE_NAME "llvmtcl")
set (PACKAGE_VERSION "3.6")
set (PKG_LIB_FILE "llvmtcl.dll")

include_directories(${LLVM_INCLUDE_DIRS} ${TCL_INCLUDE_PATH})
add_definitions(${LLVM_DEFINITIONS} "-DUSE_TCL_STUBS" "-DPACKAGE_NAME=\"${PACKAGE_NAME}\"" "-DPACKAGE_VERSION=\"${PACKAGE_VERSION}\"")

add_subdirectory("generic")

# create the pkgIndex.tcl files

configure_file("pkgIndex.tcl.in" "pkgIndex.tcl" @ONLY)

# Now build our tools


