// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/timed_lock_alg/mutex.hpp>
#include "mock_timed_mutex.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <mutex>
#include <system_error>

using namespace std::chrono_literals;
namespace tla   = beman::timed_lock_alg;
using MockMutex = beman::timed_lock_alg::test::MockTimedMutex;

// ============================================================================
// Mock Mutex Verification
// ============================================================================

TEST(MockMutex, StdLockWorks) {
    MockMutex m1, m2, m3;
    std::lock(m1, m2, m3);
    EXPECT_EQ(1, m1.lock_count);
    EXPECT_EQ(1, m2.lock_count);
    EXPECT_EQ(1, m3.lock_count);
    m1.unlock();
    m2.unlock();
    m3.unlock();
}

// ============================================================================
// Constructor Tests
// ============================================================================

TEST(MultiLock, DefaultConstructor) {
    tla::multi_lock<MockMutex> lock;
    EXPECT_FALSE(lock.owns_lock());
    EXPECT_FALSE(lock);
}

TEST(MultiLock, ZeroMutexDefaultConstructor) {
    tla::multi_lock<> lock;
    EXPECT_FALSE(lock.owns_lock());
    EXPECT_FALSE(lock);
}

TEST(MultiLock, ExplicitConstructorOneMutex) {
    MockMutex       m;
    tla::multi_lock lock(m);
    EXPECT_TRUE(lock.owns_lock());
    EXPECT_EQ(1, m.lock_count);
}

TEST(MultiLock, ExplicitConstructorMultipleMutexes) {
    MockMutex       m1, m2, m3;
    tla::multi_lock lock(m1, m2, m3);
    EXPECT_TRUE(lock.owns_lock());
    EXPECT_EQ(1, m1.lock_count);
    EXPECT_EQ(1, m2.lock_count);
    EXPECT_EQ(1, m3.lock_count);
}

TEST(MultiLock, DeferLockConstructor) {
    MockMutex       m1, m2;
    tla::multi_lock lock(std::defer_lock, m1, m2);
    EXPECT_FALSE(lock.owns_lock());
    EXPECT_EQ(0, m1.lock_count);
    EXPECT_EQ(0, m2.lock_count);
}

TEST(MultiLock, TryToLockConstructorSuccess) {
    MockMutex       m1, m2;
    tla::multi_lock lock(std::try_to_lock, m1, m2);
    EXPECT_TRUE(lock.owns_lock());
    EXPECT_EQ(1, m1.try_lock_count);
    EXPECT_EQ(1, m2.try_lock_count);
}

TEST(MultiLock, TryToLockConstructorFailure) {
    MockMutex m1, m2;
    m2.should_fail = true;
    tla::multi_lock lock(std::try_to_lock, m1, m2);
    EXPECT_FALSE(lock.owns_lock());
}

TEST(MultiLock, AdoptLockConstructor) {
    MockMutex m1, m2;
    m1.lock();
    m2.lock();
    tla::multi_lock lock(std::adopt_lock, m1, m2);
    EXPECT_TRUE(lock.owns_lock());
    EXPECT_EQ(1, m1.lock_count);
    EXPECT_EQ(1, m2.lock_count);
}

TEST(MultiLock, TimedConstructorDuration) {
    MockMutex       m1, m2;
    tla::multi_lock lock(100ms, m1, m2);
    EXPECT_TRUE(lock.owns_lock());
    EXPECT_EQ(1, m1.try_lock_count);
    EXPECT_EQ(1, m2.try_lock_count);
}

TEST(MultiLock, TimedConstructorTimePoint) {
    MockMutex       m1, m2;
    auto            tp = std::chrono::steady_clock::now() + 100ms;
    tla::multi_lock lock(tp, m1, m2);
    EXPECT_TRUE(lock.owns_lock());
    EXPECT_EQ(1, m1.try_lock_count);
    EXPECT_EQ(1, m2.try_lock_count);
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

TEST(MultiLock, MoveConstructor) {
    MockMutex       m1, m2;
    tla::multi_lock lock1(m1, m2);
    EXPECT_TRUE(lock1.owns_lock());

    tla::multi_lock lock2(std::move(lock1));
    EXPECT_TRUE(lock2.owns_lock());
    EXPECT_FALSE(lock1.owns_lock());
}

TEST(MultiLock, MoveAssignment) {
    MockMutex       m1, m2, m3, m4;
    tla::multi_lock lock1(m1, m2);
    tla::multi_lock lock2(std::defer_lock, m3, m4);

    lock2 = std::move(lock1);
    EXPECT_TRUE(lock2.owns_lock());
    EXPECT_FALSE(lock1.owns_lock());
}

// ============================================================================
// Locking Operations Tests
// ============================================================================

TEST(MultiLock, LockOneMutex) {
    MockMutex       m;
    tla::multi_lock lock(std::defer_lock, m);
    lock.lock();
    EXPECT_TRUE(lock.owns_lock());
    EXPECT_EQ(1, m.lock_count);
}

TEST(MultiLock, LockMultipleMutexes) {
    MockMutex       m1, m2, m3;
    tla::multi_lock lock(std::defer_lock, m1, m2, m3);
    lock.lock();
    EXPECT_TRUE(lock.owns_lock());
    EXPECT_EQ(1, m1.lock_count);
    EXPECT_EQ(1, m2.lock_count);
    EXPECT_EQ(1, m3.lock_count);
}

TEST(MultiLock, LockThrowsWhenAlreadyLocked) {
    MockMutex       m;
    tla::multi_lock lock(m);
    EXPECT_THROW(lock.lock(), std::system_error);
}

TEST(MultiLock, LockThrowsWhenNoMutex) {
    tla::multi_lock<MockMutex> lock;
    EXPECT_THROW(lock.lock(), std::system_error);
}

TEST(MultiLock, TryLockOneMutexSuccess) {
    MockMutex       m;
    tla::multi_lock lock(std::defer_lock, m);
    EXPECT_EQ(-1, lock.try_lock());
    EXPECT_TRUE(lock.owns_lock());
    EXPECT_EQ(1, m.try_lock_count);
}

TEST(MultiLock, TryLockOneMutexFailure) {
    MockMutex m;
    m.should_fail = true;
    tla::multi_lock lock(std::defer_lock, m);
    EXPECT_EQ(0, lock.try_lock());
    EXPECT_FALSE(lock.owns_lock());
}

TEST(MultiLock, TryLockMultipleMutexesSuccess) {
    MockMutex       m1, m2, m3;
    tla::multi_lock lock(std::defer_lock, m1, m2, m3);
    EXPECT_EQ(-1, lock.try_lock());
    EXPECT_TRUE(lock.owns_lock());
}

TEST(MultiLock, TryLockMultipleMutexesFailure) {
    MockMutex m1, m2, m3;
    m2.should_fail = true;
    tla::multi_lock lock(std::defer_lock, m1, m2, m3);
    int             result = lock.try_lock();
    EXPECT_NE(-1, result);
    EXPECT_FALSE(lock.owns_lock());
}

TEST(MultiLock, TryLockThrowsWhenAlreadyLocked) {
    MockMutex       m;
    tla::multi_lock lock(m);
    EXPECT_THROW(lock.try_lock(), std::system_error);
}

TEST(MultiLock, TryLockForSuccess) {
    MockMutex       m1, m2;
    tla::multi_lock lock(std::defer_lock, m1, m2);
    EXPECT_EQ(-1, lock.try_lock_for(100ms));
    EXPECT_TRUE(lock.owns_lock());
}

TEST(MultiLock, TryLockForFailure) {
    MockMutex m1, m2;
    m2.should_fail = true;
    tla::multi_lock lock(std::defer_lock, m1, m2);
    int             result = lock.try_lock_for(100ms);
    EXPECT_NE(-1, result);
    EXPECT_FALSE(lock.owns_lock());
}

TEST(MultiLock, TryLockUntilSuccess) {
    MockMutex       m1, m2;
    tla::multi_lock lock(std::defer_lock, m1, m2);
    auto            tp = std::chrono::steady_clock::now() + 100ms;
    EXPECT_EQ(-1, lock.try_lock_until(tp));
    EXPECT_TRUE(lock.owns_lock());
}

TEST(MultiLock, TryLockUntilFailure) {
    MockMutex m1, m2;
    m2.should_fail = true;
    tla::multi_lock lock(std::defer_lock, m1, m2);
    auto            tp     = std::chrono::steady_clock::now() + 100ms;
    int             result = lock.try_lock_until(tp);
    EXPECT_NE(-1, result);
    EXPECT_FALSE(lock.owns_lock());
}

TEST(MultiLock, UnlockSuccess) {
    MockMutex       m1, m2;
    tla::multi_lock lock(m1, m2);
    lock.unlock();
    EXPECT_FALSE(lock.owns_lock());
    EXPECT_EQ(1, m1.unlock_count);
    EXPECT_EQ(1, m2.unlock_count);
}

TEST(MultiLock, UnlockThrowsWhenNotLocked) {
    MockMutex       m;
    tla::multi_lock lock(std::defer_lock, m);
    EXPECT_THROW(lock.unlock(), std::system_error);
}

// ============================================================================
// Destructor Tests
// ============================================================================

TEST(MultiLock, DestructorUnlocksWhenOwning) {
    MockMutex m1, m2;
    {
        tla::multi_lock lock(m1, m2);
        EXPECT_TRUE(lock.owns_lock());
    }
    EXPECT_EQ(1, m1.unlock_count);
    EXPECT_EQ(1, m2.unlock_count);
}

TEST(MultiLock, DestructorDoesNotUnlockWhenNotOwning) {
    MockMutex m;
    {
        tla::multi_lock lock(std::defer_lock, m);
        EXPECT_FALSE(lock.owns_lock());
    }
    EXPECT_EQ(0, m.unlock_count);
}

// ============================================================================
// Modifiers Tests
// ============================================================================

TEST(MultiLock, Swap) {
    MockMutex       m1, m2, m3, m4;
    tla::multi_lock lock1(m1, m2);
    tla::multi_lock lock2(std::defer_lock, m3, m4);

    lock1.swap(lock2);
    EXPECT_FALSE(lock1.owns_lock());
    EXPECT_TRUE(lock2.owns_lock());
}

TEST(MultiLock, SwapFreeFunction) {
    MockMutex       m1, m2, m3, m4;
    tla::multi_lock lock1(m1, m2);
    tla::multi_lock lock2(std::defer_lock, m3, m4);

    swap(lock1, lock2);
    EXPECT_FALSE(lock1.owns_lock());
    EXPECT_TRUE(lock2.owns_lock());
}

TEST(MultiLock, Release) {
    MockMutex       m1, m2;
    tla::multi_lock lock(m1, m2);
    auto            mtxs = lock.release();
    EXPECT_FALSE(lock.owns_lock());
    EXPECT_EQ(&m1, std::get<0>(mtxs));
    EXPECT_EQ(&m2, std::get<1>(mtxs));
}

TEST(MultiLock, ReleaseDoesNotUnlock) {
    MockMutex m1, m2;
    {
        tla::multi_lock lock(m1, m2);
        lock.release();
    }
    EXPECT_EQ(0, m1.unlock_count);
    EXPECT_EQ(0, m2.unlock_count);
}

// ============================================================================
// Observers Tests
// ============================================================================

TEST(MultiLock, OwnsLock) {
    MockMutex       m;
    tla::multi_lock lock(std::defer_lock, m);
    EXPECT_FALSE(lock.owns_lock());
    lock.lock();
    EXPECT_TRUE(lock.owns_lock());
}

TEST(MultiLock, BoolOperator) {
    MockMutex       m;
    tla::multi_lock lock(std::defer_lock, m);
    EXPECT_FALSE(static_cast<bool>(lock));
    lock.lock();
    EXPECT_TRUE(static_cast<bool>(lock));
}

TEST(MultiLock, MutexAccessor) {
    MockMutex       m1, m2;
    tla::multi_lock lock(std::defer_lock, m1, m2);
    auto            mtxs = lock.mutex();
    EXPECT_EQ(&m1, std::get<0>(mtxs));
    EXPECT_EQ(&m2, std::get<1>(mtxs));
}

// ============================================================================
// Integration Tests with Real Mutexes
// ============================================================================

TEST(MultiLock, RealMutexBasicLocking) {
    std::mutex      m1, m2;
    tla::multi_lock lock(m1, m2);
    EXPECT_TRUE(lock.owns_lock());
}

TEST(MultiLock, RealTimedMutexTryLockFor) {
    std::timed_mutex m1, m2;
    tla::multi_lock  lock(10ms, m1, m2);
    EXPECT_TRUE(lock.owns_lock());
}

// ============================================================================
// Custom TrivialClock support
// ============================================================================

namespace {
struct CustomClock {
    using rep                       = std::chrono::steady_clock::rep;
    using period                    = std::chrono::steady_clock::period;
    using duration                  = std::chrono::duration<rep, period>;
    using time_point                = std::chrono::time_point<CustomClock, duration>;
    static constexpr bool is_steady = true;

    static time_point now() noexcept { return time_point{std::chrono::steady_clock::now().time_since_epoch()}; }
};
} // namespace

TEST(MultiLock, CustomClockTryLockUntilMock) {
    MockMutex       m1, m2;
    tla::multi_lock lock(std::defer_lock, m1, m2);
    EXPECT_EQ(-1, lock.try_lock_until(CustomClock::now()));
    EXPECT_TRUE(lock.owns_lock());
}

TEST(MultiLock, CustomClockConstructorMock) {
    MockMutex       m1, m2;
    tla::multi_lock lock(CustomClock::now(), m1, m2);
    EXPECT_TRUE(lock.owns_lock());
}

TEST(MultiLock, CustomClockRealTimedMutex) {
    std::timed_mutex m1, m2;
    tla::multi_lock  lock(CustomClock::now() + 10ms, m1, m2);
    EXPECT_TRUE(lock.owns_lock());
}
