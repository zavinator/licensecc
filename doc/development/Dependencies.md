# Dependencies

This page describes the dependencies of `licensecc`, the supported build environments, and the
CMake options that affect the build. It is the single source of truth: the per-platform build
pages (`Build-the-library`, `Build-the-library-windows`) link here instead of repeating version
numbers and flag tables.

Dependencies vary with the environment. If you're building the library for the first time we suggest
you set up one of the supported environments to avoid dependency/compiler errors (you can use virtual
machines, docker or lxc/lxd technologies).

The project is composed of two modules: a license generator `lccgen` executable, that also works as a
project configuration tool, and the C++ library itself `licensecc` (the part you have to integrate in
your application).

## Library `licensecc` dependencies
These are the dependencies of the library you link to your code. We try to keep them at a minimum.
N.B. Boost is always optional and it's never linked into your application.

| Operating System      | OpenSSL                  | Other                        | Boost<sup>2</sup>        |
|-----------------------|:------------------------:|:----------------------------:|:------------------------:|
| Ubuntu 26.04          | ✅ >= v1.1.1             | `libjitterentropy3-dev`      | optional (test) >= 1.65  |
| Ubuntu 24.04          | ✅ >= v1.1.1             |                              | optional (test) >= 1.65  |
| Ubuntu 22.04          | ✅ >= v1.1.1             |                              | optional (test) >= 1.65  |
| Windows MSVC 2022     | optional<sup>1</sup>     |                              | optional (test) >= 1.65  |
| Windows MinGW<sup>4</sup> | optional<sup>1</sup> |                              | optional (test) >= 1.65  |

You may have to install ZLib if your OpenSSL version was compiled with it; on Debian it comes via the
dependency mechanism.

## License generator executable `lccgen` dependencies

| Operating System          | OpenSSL               | Other                     | Boost<sup>3</sup>   |
|---------------------------|:---------------------:|:-------------------------:|:-------------------:|
| Ubuntu 26.04              | ✅ >= v1.1.1          | `libjitterentropy3-dev`   | ✅ >= 1.65          |
| Ubuntu 24.04              | ✅ >= v1.1.1          |                           | ✅ >= 1.65          |
| Ubuntu 22.04              | ✅ >= v1.1.1          |                           | ✅ >= 1.65          |
| Windows MSVC 2022         | optional<sup>1</sup>  |                           | ✅ >= 1.65          |
| Windows MinGW<sup>4</sup> | optional<sup>1</sup>  |                           | ✅ >= 1.65          |

## CMake options

The option tables previously spread across the per-platform build pages are consolidated here.
Options noted "not on Windows" are ignored/not applicable on that platform.

| Definition name              | Description |
|------------------------------|-------------|
| `BOOST_ROOT <dir>`           | Folder where Boost is installed. Not needed if Boost is installed with the system package manager. Boost is used only by the tests and the inspector, never by `liblicensecc` itself. If CMake reports "Boost not found", consider updating CMake. |
| `BUILD_SHARED_LIBS`          | Build the shared (DLL/SO) variant of the library in addition to the static one. Requires `STATIC_RUNTIME=OFF`. Default OFF. |
| `CMAKE_BUILD_TYPE`           | Build configuration, use `Release` for a release build (should be used as default). |
| `CMAKE_INSTALL_PREFIX`       | Folder where compiled libraries and headers are installed (default `/usr/local` on Linux). |
| `LCC_LOCATION <path>`        | If you download the license generator separately, the folder where it was installed or where its `lccgen-config.cmake` can be found. |
| `LCC_PROJECT_NAME <str>`     | Name of the software you want to protect. It is included in the license and used to name the project folders. If not specified, `DEFAULT` is used. |
| `LCC_PROJECTS_BASE_DIR <dir>`| Base folder where the projects are stored. Only needed if you generated a project with `lccgen` in a non-default location (not in `<<CMAKE_SOURCE_DIR>>/projects/<<PROJECT_NAME>>`). Not on Windows. |
| `OPENSSL_ROOT_DIR <dir>`     | Folder where OpenSSL is installed (e.g. `C:\Program Files\OpenSSL-Win64` on Windows). Not needed if OpenSSL is installed as a system package. |
| `OPENSSL_USE_STATIC_LIBS`    | Link against the static or dynamic version of the OpenSSL libraries (if OpenSSL is selected). Default ON. |
| `STATIC_RUNTIME`             | Link statically to the C/C++ runtime libraries (`/MT` on Windows, `-static` on Linux). Default OFF on Linux, ON on Windows. |
| `USE_OPENSSL`                | Enable/Disable OpenSSL support. On Linux it is mandatory and cannot be disabled (default ON). On Windows the Windows cryptography APIs are used instead and this defaults OFF; enabling it adds no additional feature. |

Notes:
<sup>1</sup> There is no added feature in compiling `licensecc` with OpenSSL under Windows; do so only if it's
already part of your project, otherwise go without.

<sup>2</sup> Boost components required to run tests: `unit_test_framework`, `system` (Boost < 1.90), `filesystem`.
If you don't want to install Boost, since `licensecc` uses `lccgen` in the build process you need to download
and install `lccgen` separately.

<sup>3</sup> Boost is a mandatory dependency of the `lccgen` executable. Components: `unit_test_framework`,
`system` (Boost < 1.90), `filesystem`.

<sup>4</sup> MinGW (native and Linux cross-compile) is not covered by CI in `licensecc` 2.5.0; recipes are
provided on a best-effort basis in the `Build - Linux` (cross-compile) and `Build - Windows` pages.