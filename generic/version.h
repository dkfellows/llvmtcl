#ifndef LLVMTCL_VERSION_H
#define LLVMTCL_VERSION_H

/*
 * You need to include something that defines LLVM_VERSION_MAJOR and
 * LLVM_VERSION_MINOR before this file. So pretty much any LLVM header will do
 * it.
 */

#if LLVM_VERSION_MAJOR > 3
#define API_4 1
#define API_3 1
#define API_2 1
#define API_1 1
#elif LLVM_VERSION_MAJOR == 3
#if LLVM_VERSION_MINOR > 7
#define API_3 1
#define API_2 1
#define API_1 1
#elif LLVM_VERSION_MINOR == 7
#define API_2 1
#define API_1 1
#else // LLVM_VERSION_MINOR < 7
#define API_1 1
#endif // LLVM_VERSION_MINOR >= 7
#else // LLVM_VERSION_MAJOR < 3
#error Must be at least LLVM 3.0
#endif // LLVM_VERSION_MAJOR > 3

#endif // LLVMTCL_VERSION_H

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * End:
 */
