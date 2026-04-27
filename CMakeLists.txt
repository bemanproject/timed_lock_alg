# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.30...4.3)

project(
    beman.timed_lock_alg # CMake Project Name, which is also the name of the top-level
    # targets (e.g., library, executable, etc.).
    DESCRIPTION "Timed lock algorithms for multiple lockables"
    LANGUAGES CXX
    VERSION 0.1.0
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

# for find of beman_install_library and configure_build_telemetry
include(infra/cmake/beman-install-library.cmake)
include(infra/cmake/BuildTelemetryConfig.cmake)

add_library(beman.timed_lock_alg INTERFACE)
add_library(beman::timed_lock_alg ALIAS beman.timed_lock_alg)

target_sources(
    beman.timed_lock_alg
    PUBLIC FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_target_properties(
    beman.timed_lock_alg
    PROPERTIES VERIFY_INTERFACE_HEADER_SETS ${PROJECT_IS_TOP_LEVEL}
)

add_subdirectory(include/beman/timed_lock_alg)

beman_install_library(beman.timed_lock_alg TARGETS beman.timed_lock_alg)
configure_build_telemetry()

if(BEMAN_TIMED_LOCK_ALG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/beman/timed_lock_alg)
endif()

if(BEMAN_TIMED_LOCK_ALG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
