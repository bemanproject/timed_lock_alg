// SPDX-License-Identifier: MIT

#include <beman/timed_lock_alg/mutex.hpp>

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <functional>
#include <future>
#include <tuple>

using namespace std::chrono_literals;
namespace tla = beman::timed_lock_alg;

namespace {
const auto now         = std::chrono::steady_clock::now();
const auto no_duration = std::chrono::milliseconds{0};

template <class MutexType, std::size_t N>
void unlocker(std::array<MutexType, N>& mtxs) {
    std::apply([](auto&... mts) { return std::scoped_lock(std::adopt_lock, mts...); }, mtxs);
}
} // namespace

TEST(Mutex, try_zero) {
    EXPECT_EQ(-1, tla::try_lock_until(now));
    EXPECT_EQ(-1, tla::try_lock_for(no_duration));
}

TEST(Mutex, try_one_unlocked) {
    std::timed_mutex mtx;

    EXPECT_EQ(-1, tla::try_lock_until(now, mtx));
    mtx.unlock();

    EXPECT_EQ(-1, tla::try_lock_for(no_duration, mtx));
    mtx.unlock();
}

TEST(Mutex, try_many_unlocked) {
    std::array<std::timed_mutex, 30> mtxs;

    EXPECT_EQ(-1, std::apply([](auto&... mts) { return tla::try_lock_until(now, mts...); }, mtxs));
    unlocker(mtxs);

    EXPECT_EQ(-1, std::apply([](auto&... mts) { return tla::try_lock_for(no_duration, mts...); }, mtxs));
    unlocker(mtxs);
}

TEST(Mutex, try_many_one_locked) {
    std::array<std::timed_mutex, 30> mtxs;
    auto                             th = std::jthread([&] {
        std::lock_guard lg(mtxs.back());
        std::this_thread::sleep_for(15ms);
    });

    std::this_thread::sleep_for(5ms); // approx 10ms left on lock after this
    EXPECT_EQ(-1, std::apply([](auto&... mts) { return tla::try_lock_for(20ms, mts...); }, mtxs));

    unlocker(mtxs);
}

TEST(Mutex, return_last_failed) {
    std::array<std::timed_mutex, 2> mtxs;
    auto                            th = std::jthread([&] {
        std::lock(mtxs[0], mtxs[1]);
        std::this_thread::sleep_for(10ms);
        mtxs[0].unlock(); // 5ms after try_lock_for started, 15ms left

        // try_lock_for here hangs on mtxs[1] and should return 1:
        std::this_thread::sleep_for(20ms);
        mtxs[1].unlock();
    });

    std::this_thread::sleep_for(5ms);
    EXPECT_EQ(1, std::apply([](auto&... mts) { return tla::try_lock_for(20ms, mts...); }, mtxs));
}

TEST(Mutex, succeed_with_three_in_tricky_sequence) {
    // The comments in this test are on implementation details.
    // A different implementation may behave differently but should
    // still succeed in locking all three in time.
    std::array<std::timed_mutex, 3> mtxs;
    auto                            th = std::jthread([&] {
        std::lock(mtxs[0], mtxs[1], mtxs[2]);
        std::this_thread::sleep_for(10ms);
        mtxs[0].unlock(); // 5ms after try_lock_for started, 15ms left
                          // try_lock_for gets this and jumps to mtxs[1]
        std::this_thread::sleep_for(5ms);
        mtxs[2].unlock(); // try_lock_for still hangs on mtxs[1]
        mtxs[0].lock();
        mtxs[1].unlock(); // try_lock_for gets this and jumps to mtxs[0]
                          // 10ms after try_lock_for started, 10ms left
        std::this_thread::sleep_for(5ms);
        mtxs[0].unlock(); // try_lock_for should have 5ms left here
    });

    std::this_thread::sleep_for(5ms);
    EXPECT_EQ(-1, std::apply([](auto&... mts) { return tla::try_lock_for(20ms, mts...); }, mtxs));
}
