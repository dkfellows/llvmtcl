[![Build Status](https://travis-ci.org/dkfellows/llvmtcl.svg?branch=master)](https://travis-ci.org/dkfellows/llvmtcl)

This is the llvmtcl extension based on the Tcl Extension Architecture (TEA).
Please see the the [Tcler's wiki page](http://wiki.tcl.tk/26308)
for more details.

This package is a freely available open source package.  You can do virtually
anything you like with it, such as modifying it, redistributing it, and selling
it either in whole or in part.  See the file [`license.terms`](./license.terms) for complete
information.

REQUIREMENTS
============

* Tcl 8.6
* LLVM 3.7, 3.8, 3.9, 4.0 or 5.0 (4.0 or later is recommended for performance reasons on large functions)

CONTENTS
========

The following is a short description of the files you will find in
the sample extension.

    Makefile.in     Makefile template.  The configure script uses this file to
                    produce the final Makefile.
    
    license.terms   License info for this package.
    
    aclocal.m4      Generated file.  Do not edit.  Autoconf uses this as input
                    when generating the final configure script.  See "tcl.m4"
                    below.
    
    configure       Generated file.  Do not edit.  This must be regenerated
                    any time configure.in or tclconfig/tcl.m4 changes.
    
    configure.in    Configure script template.  Autoconf uses this file as input
                    to produce the final configure script.
    
    pkgIndex.tcl.in Package index template.  The configure script will use
                    this file as input to create pkgIndex.tcl.
    
    llvmtcl-gen.inp Input for llvmtcl-gen.tcl. It contains reformatted function
                    declarations of the LLVM C API.
    
    llvmtcl-gen.tcl Script to generate the Tcl API to the LLVM C API. The
                    llvmtcl-gen.inp is the input for this script.
    
    generic/        This directory contains various source files, some only
                    generated during compilation (i.e., after `make`).

       llvmtcl.cpp  File containing most non-generated parts of the Tcl API to
                    the LLVM C API. Also includes some sections which can only
                    be implemented using the LLVM C++ API.

       llvmtcl.h    File containing all the internal APIs of llvmtcl.

       attributes.cpp
                    File containing the Tcl API to the LLVM attribute
                    management code.

       debuginfo.cpp
                    File containing the Tcl API to the LLVM debugging
                    information generation code.

       powidf2.cpp  File containing the implementation of one of the LLVM
                    intrinsics, needed on some platforms and with some linkers.

       testcode.cpp Code only used for testing purposes.

       version.h    Simplifies LLVM API version detection.

    llvmtcl.tcl     Scripts using the Tcl API to the LLVM C API.

    tests/          Some tests for the package.
    
    tclconfig/      This directory contains various template files that build
                    the configure script.  They should not need modification.
    
       install-sh   Program used for copying binaries and script files
                    to their install locations.
    
       tcl.m4       Collection of Tcl autoconf macros.  Included by
                    aclocal.m4 to define SC_* macros.
    
    win/            Files used for building on Windows.

UNIX BUILD (including OSX)
==========================

This is a C++ extension, so make sure to set the CC environment variable to a
c++ compiler (e.g. clang++ or g++).

Building under most UNIX systems is easy, just run the configure script
and then run make. For more information about the build process, see
the `tcl/unix/README` file in the Tcl src distribution. The following minimal
example will build and install the extension:

    $ cd llvmtcl
    $ ./configure --with-tcl=... --with-llvm-config=...
    $ make
    $ make test
    $ make install

WINDOWS BUILD
=============

Not supported, but see the `win` directory.

INSTALLATION
============

The installation of a TEA package is structure like so:

             $exec_prefix
              /       \
            lib       bin
             |         |
       PACKAGEx.y   (dependent .dll files on Windows)
             |
      pkgIndex.tcl (.so|.dll files)

The main `.so`|`.dll` library file gets installed in the versioned PACKAGE
directory, which is OK on all platforms because it will be directly
referenced with by `load` in the `pkgIndex.tcl` file.  Dependent DLL files on
Windows must go in the bin directory (or other directory on the user's
PATH) in order for them to be found.

USING llvmtcl
=============

* To load the extension into a Tcl interpreter:

        package require llvmtcl

* The package makes a `llvmtcl` ensemble command. The subcommands are the LLVM C
  API functions with LLVM trimmed from the front. All supported types,
  enumerators and functions can be found in [here](http://github.com/jdc8/llvmtcl/blob/master/llvmtcl-gen.inp).

* Functions taking a pointer argument followed by an unsigned argument to
  specified the number of elements the pointer is pointing to are converted to
  ensemble commands where the pointer and the unsigned argument are replaced by
  a Tcl list.

EXAMPLES
========

`examples/test.tcl` - Example using the LLVM API to create a factorial function

`examples/test2.tcl` - Example using the LLVM API to create a function adding 4 and 6 to an argument, optimize that function and execute it.

`examples/tebc.tcl` - Example converting Tcl into LLVM

`examples/ffidle.tcl` - Example calling functions in shared libraries

For a more extensive example of use, see the tclquadcode project: http://core.tcl.tk/tclquadcode/