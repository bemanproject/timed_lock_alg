# beman.timed\_lock\_alg : Timed lock algorithms for multiple lockables

<!--
SPDX-License-Identifier: MIT
-->

<!-- markdownlint-disable-next-line line-length -->
![Library Status](https://raw.githubusercontent.com/bemanproject/beman/refs/heads/main/images/badges/beman_badge-beman_library_production_ready_api_may_undergo_changes.svg) ![Continuous Integration Tests](https://github.com/bemanproject/timed_lock_alg/actions/workflows/ci_tests.yml/badge.svg) ![Lint Check (pre-commit)](https://github.com/bemanproject/timed_lock_alg/actions/workflows/pre-commit.yml/badge.svg) [![Coverage](https://coveralls.io/repos/github/bemanproject/timed_lock_alg/badge.svg?branch=main)](https://coveralls.io/github/bemanproject/timed_lock_alg?branch=main) ![Standard Target](https://github.com/bemanproject/beman/blob/main/images/badges/cpp29.svg) [![Compiler Explorer Example](https://img.shields.io/badge/Try%20it%20on%20Compiler%20Explorer-grey?logo=compilerexplorer&logoColor=67c52a)](https://godbolt.org/z/3reh77M53)

`beman.timed_lock_alg` implements timed lock algorithms for multiple lockables.

**Implements**: `std::try_lock_until` and `std::try_lock_for` proposed in [Timed lock algorithms for multiple lockables (P3832R0)](https://wg21.link/P3832R0).

**Status**: [Production ready. API may undergo changes.](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#production-ready-api-may-undergo-changes)

## License

`beman.timed_lock_alg` is licensed under the MIT License.

## Usage

`std::try_lock_until` and `std::try_lock_for` are a function templates usable with zero to many _TimedLockables_.

Example:
```
std::timed_mutex m1, m2;
if (std::try_lock_for(100ms, m1, m2) == -1) {
    // success
    std::scoped_lock sl(std::adopt_lock, m1, m2);
    // ...
} else {
    // failed to acquire within timeout
}
```

Full runnable examples can be found in [`examples/`](examples/).

## Dependencies

### Build Environment

This project requires at least the following to build:

* A C++ compiler that conforms to the C++17 standard or greater
* CMake 3.25 or later
* (Test Only) GoogleTest

You can disable building tests by setting CMake option
[`BEMAN_TIMED_LOCK_ALG_BUILD_TESTS`](#beman_timed_lock_alg_build_tests) to `OFF`
when configuring the project.

Even when tests are being built and run, some of them will not be compiled
unless the provided compiler supports **C++20** ranges.

> [!TIP]
>
> The logs indicate examples disabled due to lack of compiler support.
>
> For example:
>
> ```txt
> -- Looking for __cpp_lib_ranges
> -- Looking for __cpp_lib_ranges - not found
> CMake Warning at examples/CMakeLists.txt:12 (message):
>   Missing range support! Skip: identity_as_default_projection
>
>
> Examples to be built: identity_direct_usage
> ```

### Supported Platforms

This project officially supports:

* GCC versions 11–15
* LLVM Clang++ (with libstdc++ or libc++) versions 17–21
* AppleClang version 17.0.0 (i.e., the [latest version on GitHub-hosted macOS runners](https://github.com/actions/runner-images/blob/main/images/macos/macos-15-arm64-Readme.md))
* MSVC version 19.44.35215.0 (i.e., the [latest version on GitHub-hosted Windows runners](https://github.com/actions/runner-images/blob/main/images/windows/Windows2022-Readme.md))

> [!NOTE]
>
> Versions outside of this range would likely work as well,
> especially if you're using a version above the given range
> (e.g. HEAD/ nightly).
> These development environments are verified using our CI configuration.

## Development

### Develop using GitHub Codespace

This project supports [GitHub Codespace](https://github.com/features/codespaces)
via [Development Containers](https://containers.dev/),
which allows rapid development and instant hacking in your browser.
We recommend using GitHub codespace to explore this project as it
requires minimal setup.

Click the following badge to create a codespace:

[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/bemanproject/timed_lock_alg)

For more documentation on GitHub codespaces, please see
[this doc](https://docs.github.com/en/codespaces/).

> [!NOTE]
>
> The codespace container may take up to 5 minutes to build and spin-up; this is normal.

### Develop locally on your machines

<details>
<summary> For Linux </summary>

Beman libraries require [recent versions of CMake](#build-environment),
we recommend downloading CMake directly from [CMake's website](https://cmake.org/download/)
or installing it with the [Kitware apt library](https://apt.kitware.com/).

A [supported compiler](#supported-platforms) should be available from your package manager.

</details>

<details>
<summary> For MacOS </summary>

Beman libraries require [recent versions of CMake](#build-environment).
Use [`Homebrew`](https://brew.sh/) to install the latest version of CMake.

```bash
brew install cmake
```

A [supported compiler](#supported-platforms) is also available from brew.

For example, you can install the latest major release of Clang as:

```bash
brew install llvm
```

</details>

<details>
<summary> For Windows </summary>

To build Beman libraries, you will need the MSVC compiler. MSVC can be obtained
by installing Visual Studio; the free Visual Studio 2022 Community Edition can
be downloaded from
[Microsoft](https://visualstudio.microsoft.com/vs/community/).

After Visual Studio has been installed, you can launch "Developer PowerShell for
VS 2022" by typing it into Windows search bar. This shell environment will
provide CMake, Ninja, and MSVC, allowing you to build the library and run the
tests.

Note that you will need to use FetchContent to build GoogleTest. To do so,
please see the instructions in the "Build GoogleTest dependency from github.com"
dropdown in the [Project specific configure
arguments](#project-specific-configure-arguments) section.

</details>

### Configure and Build the Project Using CMake Presets

This project recommends using [CMake Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
to configure, build and test the project.
Appropriate presets for major compilers have been included by default.
You can use `cmake --list-presets` to see all available presets.

Here is an example to invoke the `gcc-debug` preset.

```shell
cmake --workflow --preset gcc-debug
```

Generally, there are two kinds of presets, `debug` and `release`.

The `debug` presets are designed to aid development, so it has debugging
instrumentation enabled and many sanitizers enabled.

> [!NOTE]
>
> The sanitizers that are enabled vary from compiler to compiler.
> See the toolchain files under ([`cmake`](cmake/)) to determine the exact configuration used for each preset.

The `release` presets are designed for production use, and
consequently have the highest optimization turned on (e.g. `O3`).

### Configure and Build Manually

If the presets are not suitable for your use-case, a traditional CMake
invocation will provide more configurability.

To configure, build and test the project with extra arguments,
you can run this set of commands.

```bash
cmake \
  -B build \
  -S . \
  -DCMAKE_CXX_STANDARD=20 \
  -DCMAKE_PREFIX_PATH=$PWD/infra/cmake \
  # Your extra arguments here.
cmake --build build
ctest --test-dir build
```

> [!IMPORTANT]
>
> Beman projects are
> [passive projects](https://github.com/bemanproject/beman/blob/main/docs/beman_standard.md#cmake),
> therefore,
> you will need to specify the C++ version via `CMAKE_CXX_STANDARD`
> when manually configuring the project.

### Finding and Fetching GTest from GitHub

If you do not have GoogleTest installed on your development system, you may
optionally configure this project to download a known-compatible release of
GoogleTest from source and build it as well.

Example commands:

```shell
cmake -B build -S . \
    -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=./infra/cmake/use-fetch-content.cmake \
    -DCMAKE_CXX_STANDARD=20
cmake --build build --target all
cmake --build build --target test
```

The precise version of GoogleTest that will be used is maintained in
`./lockfile.json`.

### Project specific configure arguments

Project-specific options are prefixed with `BEMAN_TIMED_LOCK_ALG`.
You can see the list of available options with:

```bash
cmake -LH -S . -B build | grep "BEMAN_TIMED_LOCK_ALG" -C 2
```

<details>

<summary> Details of CMake arguments. </summary>

#### `BEMAN_TIMED_LOCK_ALG_BUILD_TESTS`

Enable building tests and test infrastructure. Default: ON.
Values: `{ ON, OFF }`.

You can configure the project to have this option turned off via:

```bash
cmake -B build -S . -DCMAKE_CXX_STANDARD=20 -DBEMAN_TIMED_LOCK_ALG_BUILD_TESTS=OFF
```

> [!TIP]
> Because this project requires GoogleTest for running tests,
> disabling `BEMAN_TIMED_LOCK_ALG_BUILD_TESTS` avoids the project from
> cloning GoogleTest from GitHub.

#### `BEMAN_TIMED_LOCK_ALG_BUILD_EXAMPLES`

Enable building examples. Default: ON. Values: { ON, OFF }.


#### `BEMAN_TIMED_LOCK_ALG_INSTALL_CONFIG_FILE_PACKAGE`

Enable installing the CMake config file package. Default: ON.
Values: { ON, OFF }.

This is required so that users of `beman.timed\_lock\_alg` can use
`find_package(beman.timed_lock_alg)` to locate the library.

</details>

## Integrate beman.timed\_lock\_alg into your project

To use `beman.timed\_lock\_alg` in your C++ project,
include an appropriate `beman.timed\_lock\_alg` header from your source code.

```c++
#include <beman/timed_lock_alg/mutex.hpp>
```

> [!NOTE]
>
> `beman.timed\_lock\_alg` headers are to be included with the `beman/timed\_lock\_alg/` prefix.
> Altering include search paths to spell the include target another way (e.g.
> `#include <mutex.hpp>`) is unsupported.

The process for incorporating `beman.timed\_lock\_alg` into your project depends on the
build system being used. Instructions for CMake are provided in following sections.

### Incorporating `beman.timed\_lock\_alg` into your project with CMake

For CMake based projects,
you will need to use the `beman.timed\_lock\_alg` CMake module
to define the `beman::timed\_lock\_alg` CMake target:

```cmake
find_package(beman.timed_lock_alg REQUIRED)
```

You will also need to add `beman::timed\_lock\_alg` to the link libraries of
any libraries or executables that include `beman.timed\_lock\_alg` headers.

```cmake
target_link_libraries(yourlib PUBLIC beman::timed_lock_alg)
```
