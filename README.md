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
* LLVM 3.5 (or 3.6)
* Optional: CMake for Windows Build

CONTENTS
========

The following is a short description of the files you will find in
the sample extension.

    Makefile.in     Makefile template.  The configure script uses this file to
                    produce the final Makefile.
    
    README          This file.
    
    license.terms   License info for thie package.
    
    aclocal.m4      Generated file.  Do not edit.  Autoconf uses this as input
                    when generating the final configure script.  See "tcl.m4"
                    below.
    
    configure       Generated file.  Do not edit.  This must be regenerated
                    anytime configure.in or tclconfig/tcl.m4 changes.
    
    configure.in    Configure script template.  Autoconf uses this file as input
                    to produce the final configure script.
    
    pkgIndex.tcl.in Package index template.  The configure script will use
                    this file as input to create pkgIndex.tcl.
    
    llvmtcl-gen.inp Input for llvmtcl-gen.tcl. It contains reformatted function
                    declarations of the llvm C API.
    
    llvmtcl-gen.tcl Script to generate the Tcl API to the llvm C API. The
                    llvmtcl-gen.inp is the input for this script.
    
    generic/        This directory contains various source files, some only
                    generated during compilation (i.e., after `make`).

       llvmtcl.cpp  File containing all non-generated parts of the Tcl API to
                    the LLVM C API. Also includes some sections which can only
                    be implemented using the LLVM C++ API.

    llvmtcl.tcl     Scripts using the Tcl API to the llvm C API.
    
    tests           Some tests for the package.
    
    tclconfig/      This directory contains various template files that build
                    the configure script.  They should not need modification.
    
       install-sh   Program used for copying binaries and script files
                    to their install locations.
    
       tcl.m4       Collection of Tcl autoconf macros.  Included by
                    aclocal.m4 to define SC_* macros.

    CMakeLists.txt  CMake Build File. 

UNIX BUILD
==========

This is a C++ extension, so make sure to set the CC environment variable to a
c++ compiler (e.g. g++).

Building under most UNIX systems is easy, just run the configure script
and then run make. For more information about the build process, see
the `tcl/unix/README` file in the Tcl src dist. The following minimal
example will build and install the extension:

    $ cd llvmtcl
    $ ./configure --with-tcl=... --with-llvm-config=...
    $ make
    $ make test
    $ make install

Add the llvm lib directory containing the llvm shared object files to the
`LD_LIBRARY_PATH` environment variable.

WINDOWS BUILD
=============

A basic build system based on the CMake project file generator is available.
See [CMake](http://cmake.org) for details.

You can build the llvmtcl package with VisualStudio 12 like this:

64-Bit Build
------------

You need to have built llvm-3.6 with CMake for x64 and should have an installed Tcl (e.g. ActiveTcl x64).

    c:\devel\tclllvm> md build_x64
    c:\devel\tclllvm> cd build_x64
    c:\devel\tclllvm\build_x64> "c:\Program Files (x86)\CMake\bin\cmake.exe" -G"Visual Studio 12 2013 Win64" ..
    c:\devel\tclllvm\build_x64> msbuild llvmtcl.sln /t:Build /p:Configuration=RelWithDebInfo /p:Platform="x64"

If you want to install the build, you can use the INSTALL target, which will install it in the location given
by the `CMAKE_INSTALL_PREFIX` in a subdir like `lib/llvmtcl3.6`.

32-Bit Build
------------
You need to have built llvm-3.6 with CMake for x86 and should have an installed Tcl (e.g. ActiveTcl win32).

    c:\devel\tclllvm> md build_x86
    c:\devel\tclllvm> cd build_x86
    c:\devel\tclllvm\build_x86> "c:\Program Files (x86)\CMake\bin\cmake.exe" -G"Visual Studio 12 2013" ..
    c:\devel\tclllvm\build_x86> msbuild llvmtcl.sln /t:Build /p:Configuration=RelWithDebInfo /p:Platform="Win32"

If you want to install the build, you can use the INSTALL target, which will install it in the location given
by the `CMAKE_INSTALL_PREFIX` in a subdir like `lib/llvmtcl3.6`.

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
