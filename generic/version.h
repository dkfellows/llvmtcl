/*
 * version.h --
 *
 *	This file contains the preprocessor macros to determine what LLVM API
 *	version is being used, as observable by llvmtcl.
 *
 * Copyright (c) 2017-2018 Donal K. Fellows.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef LLVMTCL_VERSION_H
#define LLVMTCL_VERSION_H

/*
 * You need to include something that defines LLVM_VERSION_MAJOR and
 * LLVM_VERSION_MINOR before this file. So pretty much any LLVM header will do
 * it.
 */

#if LLVM_VERSION_MAJOR <= 3
#error This version of LLVM is no longer supported; upgrade to 4.0 or later
#endif // LLVM_VERSION_MAJOR > 3

#if LLVM_VERSION_MAJOR > 7
#define API_8 1
#endif // LLVM_VERSION_MAJOR > 4

#if LLVM_VERSION_MAJOR > 6
#define API_7 1
#endif // LLVM_VERSION_MAJOR > 4

#if LLVM_VERSION_MAJOR > 5
#define API_6 1
#endif // LLVM_VERSION_MAJOR > 4

#if LLVM_VERSION_MAJOR > 4
#define API_5 1
#endif // LLVM_VERSION_MAJOR > 4

#endif // LLVMTCL_VERSION_H

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * End:
 */
