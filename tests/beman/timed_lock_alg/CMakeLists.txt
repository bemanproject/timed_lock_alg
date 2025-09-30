# SPDX-License-Identifier: MIT

find_package(GTest REQUIRED)

add_executable(beman.timed_lock_alg.tests.mutex)
target_sources(beman.timed_lock_alg.tests.mutex PRIVATE mutex.test.cpp)
target_link_libraries(
    beman.timed_lock_alg.tests.mutex
    PRIVATE beman::timed_lock_alg GTest::gtest GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(beman.timed_lock_alg.tests.mutex)
