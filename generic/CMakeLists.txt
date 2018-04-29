# Generate the llvm interfaces
#
ADD_CUSTOM_COMMAND(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/llvmtcl-gen-cmddef.h ${CMAKE_CURRENT_BINARY_DIR}/generated/llvmtcl-gen-map.h ${CMAKE_CURRENT_BINARY_DIR}/generated/llvmtcl-gen.h
   COMMAND ${TCL_TCLSH} ${CMAKE_CURRENT_SOURCE_DIR}/../llvmtcl-gen.tcl "${CMAKE_CURRENT_BINARY_DIR}/.."
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../llvmtcl-gen.tcl 
           ${CMAKE_CURRENT_SOURCE_DIR}/../llvmtcl-gen.inp 
   )
ADD_CUSTOM_TARGET(generate_defs DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/generated/llvmtcl-gen-cmddef.h)

SET_SOURCE_FILES_PROPERTIES(
	${CMAKE_CURRENT_BINARY_DIR}/generated/llvmtcl-gen-cmddef.h PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(
	${CMAKE_CURRENT_BINARY_DIR}/generated/llvmtcl-gen-map.h PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(
	${CMAKE_CURRENT_BINARY_DIR}/generated/llvmtcl-gen.h PROPERTIES GENERATED 1)

ADD_DEFINITIONS(${LLVM_DEFINITIONS} "-DUSE_TCL_STUBS" "-DPACKAGE_NAME=\"${PACKAGE_NAME}\"" "-DPACKAGE_VERSION=\"${PACKAGE_VERSION}\"")
INCLUDE_DIRECTORIES(${LLVM_INCLUDE_DIRS} ${TCL_INCLUDE_PATH} ${CMAKE_CURRENT_BINARY_DIR})


ADD_LIBRARY(llvmtcl SHARED llvmtcl.cpp)
# depend on the generated files above
ADD_DEPENDENCIES(llvmtcl generate_defs)

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs all)
TARGET_LINK_LIBRARIES(llvmtcl ${llvm_libs} ${TCL_STUB_LIBRARY})

INSTALL(TARGETS llvmtcl	DESTINATION "lib/${PACKAGE_NAME}${PACKAGE_VERSION}")
