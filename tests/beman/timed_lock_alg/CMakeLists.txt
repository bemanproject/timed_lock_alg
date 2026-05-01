# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

find_package(GTest REQUIRED)

add_executable(beman.timed_lock_alg.tests.try_lock)
target_sources(beman.timed_lock_alg.tests.try_lock PRIVATE try_lock.test.cpp)
target_link_libraries(
    beman.timed_lock_alg.tests.try_lock
    PRIVATE beman::timed_lock_alg GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(beman.timed_lock_alg.tests.try_lock DISCOVERY_TIMEOUT 60)

add_executable(beman.timed_lock_alg.tests.multi_lock)
target_sources(
    beman.timed_lock_alg.tests.multi_lock
    PRIVATE multi_lock.test.cpp
)
target_link_libraries(
    beman.timed_lock_alg.tests.multi_lock
    PRIVATE beman::timed_lock_alg GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(beman.timed_lock_alg.tests.multi_lock DISCOVERY_TIMEOUT 60)
