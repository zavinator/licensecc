# Build - Linux

For quick compilation instructions see the [`README.md`](https://github.com/open-license-manager/licensecc#how-to-build) file in the repository root. This page provides detailed, platform-specific instructions for Linux.

## Install prerequisites
Install the prerequisites listed here. For a complete, always-current list of dependencies and
supported environments see the [Dependencies](Dependencies) page (single source of truth).

### Ubuntu
Supported Ubuntu distributions are 26.04, 24.04 (Noble Numbat) and 22.04 (Jammy Jellyfish), on both
x86_64 and ARM. It should be possible to build on any recent Debian-derivative distribution.

Install prerequisites (Ubuntu 24.04 and 22.04):

```console
sudo apt-get install cmake valgrind libssl-dev zlib1g-dev libboost-test-dev libboost-filesystem-dev \
     libboost-iostreams-dev libboost-program-options-dev libboost-system-dev libboost-thread-dev \
     libboost-date-time-dev build-essential
```

Ubuntu 26.04 additionally requires `libjitterentropy3-dev` (its OpenSSL 3.x links against the Jitter
RNG entropy source):

```console
sudo apt-get install libjitterentropy3-dev
```

### Other linux
Licensecc should compile on any recent Linux distribution. For the minimum toolchain versions and for
the optional documentation dependencies see the [Dependencies](Dependencies) page.

## Download and compile

### Download:
This project has a submodule (the license generator). Remember to add the option `--recursive` to clone it.

```console
git clone --recursive https://github.com/open-license-manager/licensecc.git
```

### Configure:

```console
cd licensecc/build
cmake .. -DCMAKE_INSTALL_PREFIX=../install
```

### Compile and test:

```console
make
make install
```

```console
make test
ctest -T memcheck
```

For the full list of CMake options (including `BOOST_ROOT`, `STATIC_RUNTIME`, `USE_OPENSSL` and the
project-related variables) see the [Dependencies](Dependencies) page.

## Cross compile on Linux for Windows
The procedure below builds a Windows library from Linux using the MinGW cross toolchain. It is not
covered by CI (Windows builds are produced natively, see
`.github/workflows/windows-standard.yml`), so treat it as best-effort.

### Prerequisites

The cross compiler and the standard Windows C/C++ runtime are provided by `mingw-w64`. The build also
takes the toolchain file shipped in the repository:
`cmake/toolchain-ubuntu-mingw64.cmake`.

```console
sudo apt-get install cmake mingw-w64 g++-mingw-w64 binutils-mingw-w64 \
	mingw-w64-x86-64-dev libz-mingw-w64-dev p7zip-full
```

Download and compile Boost (replace `1.85.0` with a currently supported release; source archives are
published on the `boostorg/boost` GitHub releases page):

```console
export CUR_PATH=$(pwd)
wget -c https://github.com/boostorg/boost/releases/download/boost-1.85.0/boost-1.85.0.tar.bz2
tar xjf boost-1.85.0.tar.bz2
cd boost-1.85.0
./bootstrap.sh
./b2 toolset=gcc-mingw target-os=windows address-model=64 --with-date_time --with-test \
    --with-filesystem --with-program_options --with-regex --with-serialization --with-system \
    runtime-link=static --prefix=./dist release install
cd "$CUR_PATH"
```

Install OpenSSL for MinGW. Cross-compile it from source:

```console
wget https://www.openssl.org/source/openssl-3.0.15.tar.gz
tar xzf openssl-3.0.15.tar.gz
cd openssl-3.0.15
./Configure no-zlib no-shared --prefix="$PWD/dist-win-64" \
    --cross-compile-prefix=x86_64-w64-mingw32- mingw64
make -j"$(nproc)"
make install_sw
cd "$CUR_PATH"
```

Configure and compile `licensecc`, pointing at the cross-compiled Boost and OpenSSL:

```console
cd licensecc/build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-ubuntu-mingw64.cmake \
    -DOPENSSL_ROOT_DIR="$CUR_PATH/openssl-3.0.15/dist-win-64" \
    -DOPENSSL_USE_STATIC_LIBS=ON \
    -DBOOST_ROOT="$CUR_PATH/boost-1.85.0/dist" \
    -DBoost_ARCHITECTURE=-x64 ..
cmake --build . --target install --config Release
```

### Build documentation

Setup the python virtual environment:

```console
python3 -m venv .venv

. .venv/bin/activate
pip install wheel
pip install -r requirements.txt
```

Build the docs:

```console
. .venv/bin/activate
cd build
cmake ..
make documentation
```
