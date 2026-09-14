#######################################
Build - Windows
#######################################

This page describes how to build the library under windows with licensecc 2.5.0.

For quick compilation instructions see the `README.md <https://github.com/open-license-manager/licensecc#how-to-build>`_ file in the repository root. This page provides detailed, platform-specific instructions for Windows.

MSVC
*****************

Supported Visual Studio versions are:

* Visual Studio 2026 (used in development).
* Visual Studio 2022 (used in the automated builds). The lowest
  supported version is the latest minor release/servicing update of Visual Studio 2022
  (v17.x).
* Visual Studio 2019 and 2017 are not tested anymore (though they should still work).

Automated tests run on Windows Server 2022, Windows Server 2026 and Windows 11 (arm64).

Libraries supported/tested in 2.5.0 (Windows x64):

* CMake: >= 3.16.
* Boost: tested with 1.64.0, 1.78.0 and 1.90.0. Boost is only needed to run the tests
  and to build ``lccgen`` and ``lcc-inspector``; it's never linked into ``liblicensecc``.
  The pre-compiled binaries should match the compiler version.
* OpenSSL: optional. 

MSVC install prerequisites
============================= 
Git is of course a prerequisite, if you don't have it you can download it from `git-scm.com <https://git-scm.com/download/win>`_. 

Pre-compiled versions of boost for windows can be downloaded from the
`userdocs/boost <https://github.com/userdocs/boost/releases>`_ releases (the same source
used by the project CI). Choose the installer that matches the desired architecture and
compiler, eg. for Visual Studio 2022 64 bit download ``boost_1_90_0-msvc-14.3-64.exe``.

Alternatively pre-compiled boost binaries are available at
`SourceForge <https://sourceforge.net/projects/boost/files/boost-binaries/>`_.

Checkout the code
==================
Check out the code using git:

.. NOTE::
 
  This project has a submodule (the license generator). Remember to add the option `--recursive` to clone it.

.. code-block:: console

  git clone --recursive https://github.com/open-license-manager/licensecc.git

Compile and build (command line)
====================================

Open a command prompt in the folder where you checked out the library.

Configure the library (windows x64):
 
.. code-block:: console
  
  cd build
  cmake .. -G "Visual Studio 17 2022" -A x64 -DBOOST_ROOT="C:\local\boost"  //(or where boost was installed)

Configure the library (windows x86):

For some configuration reason we're unable to build using x86 using visual studio generators. We recommend to use Ninja
build system. 

.. code-block:: console

  cd build
  "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86

  cmake .. -G "Ninja" [-DBOOST_ROOT="C:\local\boost"  //(or where boost was installed)]

Supported cmake definitions/options
=======================================
For a complete, always-current list of CMake options (including ``BOOST_ROOT``,
``STATIC_RUNTIME``, ``USE_OPENSSL`` and the project-related variables) see the
:doc:`Dependencies <Dependencies>` page, which is the single source of truth for both Linux and Windows.

Compile and test 

.. code-block:: console
  
  cmake --build . --target install --config Release
  ctest -C Release


Compile and build (Visual Studio 2022)
==========================================

Visual Studio 2022 integrates with CMake (the process requires a couple of restarts and it's all but "fluid").

After opening the project "as a CMake project" a CMakeSettings.json should appear in the base folder. Edit the file as 
below (the file is for an x86 architecture). In a special way:

* remove the "-v" switch from "buildCommandArgs" 
* add the variable BOOST_ROOT pointing to where your boost installation is.

Restart, delete and rebuild the cache a couple of time, until Visual Studio understands the new options.

.. literalinclude:: CMakeSettings.json
   :language: json


Windows ARM64
*****************
Windows on ARM (arm64) is supported and tested in CI on a Windows 11 arm64 runner.
Boost is provided by `vcpkg <https://vcpkg.io/>`_ (packages ``boost-program-options``,
``boost-test``, ``boost-filesystem``, ``boost-date-time`` with the
``arm64-windows-static-release`` triplet) and OpenSSL is disabled (the Windows
cryptography APIs are used):

.. code-block:: console

  cmake -S . -B build -DSTATIC_RUNTIME=ON -DUSE_OPENSSL=OFF -DCMAKE_BUILD_TYPE=Release ^
        -DCMAKE_TOOLCHAIN_FILE="<vcpkg folder>\scripts\buildsystems\vcpkg.cmake" ^
        -DVCPKG_TARGET_TRIPLET=arm64-windows-static-release
  cmake --build build --target install --config Release
  ctest -C Release --test-dir build
   

MINGW 
*****************
MinGW is not covered by CI in 2.5.0, but it worked in 2.0.0 and may work again in the future.
This section provides a best-effort recipe; if it works for you please comment in the forum.

Prerequisites:

* PowerShell
* 7-Zip
* git
* cmake
* an MSYS2 or MinGW-w64 toolchain (gcc/g++), or pre-built MinGW Boost/OpenSSL packages

The easiest way to get the dependencies is via `MSYS2 <https://www.msys2.org/>`_:

.. code-block:: console

   pacman -S mingw-w64-x86_64-toolchain \
             mingw-w64-x86_64-boost \
             mingw-w64-x86_64-openssl \
             mingw-w64-x86_64-cmake \
             mingw-w64-x86_64-ninja

Alternatively, pre-compiled Boost binaries are available from the
`boostorg/boost <https://github.com/boostorg/boost/releases>`_ releases (source) or the
`userdocs/boost <https://github.com/userdocs/boost/releases>`_ releases (binaries).

Verify the toolchain is on ``PATH``, then check out and build `licensecc`:

.. code-block:: console

   git clone --recursive https://github.com/open-license-manager/licensecc.git
   cd licensecc/build
   cmake -G "MinGW Makefiles" \
     -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
     -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
     -DBOOST_ROOT="C:/msys64/mingw64" \
     ..
   cmake --build . --target install --config Release

Adapt ``BOOST_ROOT`` (and add ``OPENSSL_ROOT_DIR`` if OpenSSL is elsewhere) to your actual
installation. Then run the tests:

.. code-block:: console

   ctest -C Release



