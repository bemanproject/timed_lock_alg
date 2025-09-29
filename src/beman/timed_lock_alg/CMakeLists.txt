# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

add_library(beman.timed_lock_alg)
add_library(beman::timed_lock_alg ALIAS beman.timed_lock_alg)

target_sources(beman.timed_lock_alg PRIVATE mutex.cpp)

target_sources(
    beman.timed_lock_alg
    PUBLIC
        FILE_SET HEADERS
            BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../include"
            FILES
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/beman/timed_lock_alg/mutex.hpp"
)

set_target_properties(beman.timed_lock_alg PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)

find_package(beman-install-library REQUIRED)
beman_install_library(beman.timed_lock_alg)
