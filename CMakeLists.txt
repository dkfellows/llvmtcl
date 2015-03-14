cmake_minimum_required(VERSION 2.8.8)
project(llvmtcl)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_package(TCL REQUIRED)
find_package(TclSh REQUIRED)
find_package(TclStub REQUIRED)

set (PACKAGE_NAME "llvmtcl")
set (PACKAGE_VERSION "3.6")
set (PKG_LIB_FILE "llvmtcl.dll")


add_subdirectory("generic")

# create the pkgIndex.tcl files

configure_file("pkgIndex.tcl.in" "pkgIndex.tcl" @ONLY)

SET (PKG_INSTALL_DIR "lib/${PACKAGE_NAME}${PACKAGE_VERSION}")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl DESTINATION ${PKG_INSTALL_DIR})
install(FILES llvmtcl.tcl optimise.tcl DESTINATION ${PKG_INSTALL_DIR})



