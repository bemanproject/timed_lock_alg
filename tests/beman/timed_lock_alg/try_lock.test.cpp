// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/timed_lock_alg/mutex.hpp>
#include "mock_timed_mutex.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <mutex>
#include <thread>
#include <tuple>

using namespace std::chrono_literals;
namespace tla   = beman::timed_lock_alg;
using MockMutex = beman::timed_lock_alg::test::MockTimedMutex;

namespace {
// joining thread for implementations missing std::jthread
class JThread : public std::thread {
  public:
    template <class... Args>
    JThread(Args&&... args) : std::thread(std::forward<Args>(args)...) {}
    ~JThread() {
        if (joinable()) {
            join();
        }
    }
};
const auto now         = std::chrono::steady_clock::now();
const auto no_duration = 0ms;
const auto extra_grace = 100ms;

template <class MutexType, std::size_t N>
void unlocker(std::array<MutexType, N>& mtxs) {
    std::apply([](auto&... mts) { return std::scoped_lock(std::adopt_lock, mts...); }, mtxs);
}

// Mutex whose try_lock_until/for spins on should_fail instead of timing out,
// allowing deterministic scripting of lock sequences without sleeps.
// wait_count increments each time a thread enters the spin, enabling the
// background thread to synchronize on "algorithm is now blocked here".
// held tracks whether the algorithm currently owns this mutex.
struct ScriptedMutex {
    std::atomic<bool> should_fail{false};
    std::atomic<int>  wait_count{0};
    std::atomic<bool> held{false};

    void lock() {
        if (should_fail) {
            ++wait_count;
            while (should_fail)
                std::this_thread::yield();
        }
        held = true;
    }
    bool try_lock() {
        if (should_fail)
            return false;
        held = true;
        return true;
    }
    template <class R, class P>
    bool try_lock_for(const std::chrono::duration<R, P>&) {
        lock();
        return true;
    }
    template <class C, class D>
    bool try_lock_until(const std::chrono::time_point<C, D>&) {
        lock();
        return true;
    }
    void unlock() { held = false; }
    void wait_for_waiter(int n = 1) {
        while (wait_count < n)
            std::this_thread::yield();
    }
};
} // namespace

// ============================================================================
// Basic Tests with Mock Mutexes (fast, deterministic)
// ============================================================================

TEST(TryLock, ZeroMutexes) {
    EXPECT_EQ(-1, tla::try_lock_until(now));
    EXPECT_EQ(-1, tla::try_lock_for(no_duration));
}

TEST(TryLock, OneMutexUnlocked) {
    MockMutex mtx;
    EXPECT_EQ(-1, tla::try_lock_until(now, mtx));
    mtx.unlock();

    EXPECT_EQ(-1, tla::try_lock_for(no_duration, mtx));
    mtx.unlock();
}

TEST(TryLock, ManyMutexesUnlocked) {
    std::array<MockMutex, 30> mtxs;

    EXPECT_EQ(-1, std::apply([](auto&... mts) { return tla::try_lock_until(now, mts...); }, mtxs));
    unlocker(mtxs);

    EXPECT_EQ(-1, std::apply([](auto&... mts) { return tla::try_lock_for(no_duration, mts...); }, mtxs));
    unlocker(mtxs);
}

TEST(TryLock, OneMutexLocked) {
    MockMutex mtx;
    mtx.should_fail = true;
    EXPECT_EQ(0, tla::try_lock_until(now, mtx));
    EXPECT_EQ(0, tla::try_lock_for(no_duration, mtx));
}

TEST(TryLock, ManyMutexesOneLockedFirst) {
    std::array<MockMutex, 3> mtxs;
    mtxs[0].should_fail = true;
    int result          = std::apply([](auto&... mts) { return tla::try_lock_for(no_duration, mts...); }, mtxs);
    EXPECT_EQ(0, result);
}

TEST(TryLock, ManyMutexesOneLockedMiddle) {
    std::array<MockMutex, 3> mtxs;
    mtxs[1].should_fail = true;
    int result          = std::apply([](auto&... mts) { return tla::try_lock_for(no_duration, mts...); }, mtxs);
    EXPECT_EQ(1, result);
}

TEST(TryLock, ManyMutexesOneLockedLast) {
    std::array<MockMutex, 3> mtxs;
    mtxs[2].should_fail = true;
    int result          = std::apply([](auto&... mts) { return tla::try_lock_for(no_duration, mts...); }, mtxs);
    EXPECT_EQ(2, result);
}

// ============================================================================
// Integration Tests with Real Mutexes (verify actual threading behavior)
// ============================================================================

TEST(TryLockIntegration, RealMutexBasic) {
    std::timed_mutex mtx;
    EXPECT_EQ(-1, tla::try_lock_until(now, mtx));
    mtx.unlock();

    EXPECT_EQ(-1, tla::try_lock_for(no_duration, mtx));
    mtx.unlock();
}

TEST(TryLockIntegration, ManyRealMutexesUnlocked) {
    std::array<std::timed_mutex, 30> mtxs;

    EXPECT_EQ(-1, std::apply([](auto&... mts) { return tla::try_lock_until(now, mts...); }, mtxs));
    unlocker(mtxs);

    EXPECT_EQ(-1, std::apply([](auto&... mts) { return tla::try_lock_for(no_duration, mts...); }, mtxs));
    unlocker(mtxs);
}

TEST(TryLockIntegration, ManyMutexesOneLockedWithTimeout) {
    std::array<std::timed_mutex, 30> mtxs;
    auto                             th = JThread([&] {
        std::lock_guard lg(mtxs.back());
        std::this_thread::sleep_for(15ms);
    });

    std::this_thread::sleep_for(5ms); // approx 10ms left on lock after this
    EXPECT_EQ(-1, std::apply([](auto&... mts) { return tla::try_lock_for(20ms + extra_grace, mts...); }, mtxs));

    unlocker(mtxs);
}

TEST(TryLockIntegration, ReturnLastFailed) {
    std::array<std::timed_mutex, 2> mtxs;
    auto                            th = JThread([&] {
        std::lock(mtxs[0], mtxs[1]);
        std::this_thread::sleep_for(100ms);
        mtxs[0].unlock(); // 50ms after try_lock_for started, 200ms left

        // try_lock_for here hangs on mtxs[1] and should return 1:
        std::this_thread::sleep_for(300ms + extra_grace);
        mtxs[1].unlock();
    });

    std::this_thread::sleep_for(50ms);
    EXPECT_EQ(1, std::apply([](auto&... mts) { return tla::try_lock_for(200ms, mts...); }, mtxs));
}

TEST(TryLockIntegration, SucceedWithThreeInTrickySequence) {
    // The comments in this test are on implementation details.
    // A different implementation may behave differently but should
    // still succeed in locking all three in time.
    //
    // Algorithm rotation with 3 mutexes [m0,m1,m2]:
    //   idx=0 [0,1,2]: try_lock_until(m0), try_lock(m1,m2) → m1 fails → retry idx=1
    //   idx=1 [1,2,0]: try_lock_until(m1), try_lock(m2,m0) → m2 fails → retry idx=2
    //   idx=2 [2,0,1]: try_lock_until(m2), try_lock(m0,m1) → m0 fails → retry idx=0
    //   idx=0 [0,1,2]: try_lock_until(m0), try_lock(m1,m2) → both succeed → done
    ScriptedMutex m0, m1, m2;
    m0.should_fail = m1.should_fail = m2.should_fail = true;

    auto th = JThread([&] {
        // idx=0: algorithm blocks on m0
        m0.wait_for_waiter(1);
        EXPECT_FALSE(m1.held);
        EXPECT_FALSE(m2.held);
        m0.should_fail = false; // unblock m0; try_lock(m1) fails → releases m0, retry idx=1

        // idx=1: algorithm blocks on m1
        m1.wait_for_waiter(1);
        EXPECT_FALSE(m0.held);
        EXPECT_FALSE(m2.held);
        m0.should_fail = true;  // re-block m0 before algorithm reaches it
        m1.should_fail = false; // unblock m1; try_lock(m2) fails → releases m1, retry idx=2

        // idx=2: algorithm blocks on m2
        m2.wait_for_waiter(1);
        EXPECT_FALSE(m0.held);
        EXPECT_FALSE(m1.held);
        m1.should_fail = true;  // re-block m1 before algorithm reaches it
        m2.should_fail = false; // unblock m2; try_lock(m0) fails → releases m2, retry idx=0

        // idx=0 again: algorithm blocks on m0
        m0.wait_for_waiter(2);
        EXPECT_FALSE(m1.held);
        EXPECT_FALSE(m2.held);
        m1.should_fail = false; // all free for final round
        m2.should_fail = false;
        m0.should_fail = false; // unblock m0; try_lock(m1,m2) → both succeed → done
    });

    EXPECT_EQ(-1, tla::try_lock_for(24h, m0, m1, m2));
}
