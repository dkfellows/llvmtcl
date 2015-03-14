ADD_CUSTOM_COMMAND(
   OUTPUT llvmtcl-gen-cmddef.c llvmtcl-gen-map.c llvmtcl-gen.c
   COMMAND ${TCL_TCLSH} ${CMAKE_CURRENT_SOURCE_DIR}/../llvmtcl-gen.tcl "${CMAKE_CURRENT_BINARY_DIR}/.."
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../llvmtcl-gen.tcl 
           ${CMAKE_CURRENT_SOURCE_DIR}/../llvmtcl-gen.inp 
   )

add_library(llvmtcl SHARED "llvmtcl.cpp")

# Link against LLVM libraries

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs all)
target_link_libraries(llvmtcl ${llvm_libs} ${TCL_STUB_LIBRARY})

SET_SOURCE_FILES_PROPERTIES(
	${CMAKE_CURRENT_BINARY_DIR}/generic/llvmtcl-gen-cmddef.c PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(
	${CMAKE_CURRENT_BINARY_DIR}/generic/llvmtcl-gen-map.c PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(
	${CMAKE_CURRENT_BINARY_DIR}/generic/llvmtcl-gen.c PROPERTIES GENERATED 1)

