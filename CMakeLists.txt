# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.25)

project(
    beman.timed_lock_alg # CMake Project Name, which is also the name of the top-level
    # targets (e.g., library, executable, etc.).
    DESCRIPTION "Beman Timed Lock Algorithms"
    LANGUAGES CXX
    VERSION 1.0.0
)

# [CMAKE.SKIP_TESTS]
option(
    BEMAN_TIMED_LOCK_ALG_BUILD_TESTS
    "Enable building tests and test infrastructure. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

# [CMAKE.SKIP_EXAMPLES]
option(
    BEMAN_TIMED_LOCK_ALG_BUILD_EXAMPLES
    "Enable building examples. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

include(CTest)

add_subdirectory(src/beman/timed_lock_alg)

if(BEMAN_TIMED_LOCK_ALG_BUILD_TESTS)
    add_subdirectory(tests/beman/timed_lock_alg)
endif()

if(BEMAN_TIMED_LOCK_ALG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
