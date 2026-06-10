// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_TIMED_LOCK_ALG_MUTEX_HPP
#define BEMAN_TIMED_LOCK_ALG_MUTEX_HPP

#include <array>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <mutex>
#include <thread>
#include <tuple>
#include <utility>

namespace beman::timed_lock_alg::detail {
template <class T>
concept BasicLockable = requires(T t) {
    t.lock();
    t.unlock();
};

template <class T>
concept Lockable = BasicLockable<T> && requires(T t) {
    { t.try_lock() } -> std::same_as<bool>;
};

template <class T>
concept TimedLockable = Lockable<T> && requires(T t) {
    { t.try_lock_for(std::chrono::nanoseconds{}) } -> std::same_as<bool>;
    { t.try_lock_for(std::chrono::microseconds{}) } -> std::same_as<bool>;
    { t.try_lock_for(std::chrono::milliseconds{}) } -> std::same_as<bool>;
    { t.try_lock_until(std::chrono::time_point<std::chrono::steady_clock>{}) } -> std::same_as<bool>;
    { t.try_lock_until(std::chrono::time_point<std::chrono::system_clock>{}) } -> std::same_as<bool>;
};
} // namespace beman::timed_lock_alg::detail

namespace beman::timed_lock_alg {
namespace detail {
template <class...>
struct type_pack {}; // seems to compile slightly faster than using a tuple
namespace rot_detail {
template <std::size_t I, class IndexSeq>
struct rotate;

template <std::size_t I, std::size_t... J>
struct rotate<I, std::index_sequence<J...>> {
    using type = std::index_sequence<((J + I) % sizeof...(J))...>;
};

template <class IndexSeq>
struct make_pack_impl;

template <std::size_t... I>
struct make_pack_impl<std::index_sequence<I...>> {
    using type = type_pack<typename rotate<I, std::index_sequence<I...>>::type...>;
};
} // namespace rot_detail

template <std::size_t N>
using make_pack_of_rotating_index_sequences = typename rot_detail::make_pack_impl<std::make_index_sequence<N>>::type;
//-------------------------------------------------------------------------
int friendly_try_lock(auto& l1) { return -static_cast<int>(l1.try_lock()); }
int friendly_try_lock(auto&... ls) { return std::try_lock(ls...); }
//-------------------------------------------------------------------------
struct result {
    int  idx;
    bool retry;
};
//-------------------------------------------------------------------------
template <class Clock, class Duration, class Locks, class... Seqs>
int try_lock_until_impl(const std::chrono::time_point<Clock, Duration>& end_time, Locks locks, type_pack<Seqs...>) {
    // an array with one function per lockable/sequence to try:
    constexpr std::array<result (*)(const std::chrono::time_point<Clock, Duration>&, Locks&), sizeof...(Seqs)> seqs{
        {+[](const std::chrono::time_point<Clock, Duration>& tp, Locks& lks) {
            return []<class Tp, std::size_t I0, std::size_t... Is>(
                       const Tp& itp, Locks& lcks, std::index_sequence<I0, Is...>) -> result {
                if (std::unique_lock first{std::get<I0>(lcks), itp}) {
                    int res = friendly_try_lock(std::get<Is>(lcks)...);
                    if (res == -1) {
                        first.release();
                        return {-1, false}; // success
                    }
                    // return the index to try_lock_until next round or the index which failed to lock
                    // in case time has run out
                    constexpr std::array idxs{static_cast<int>(Is)...};
                    return {idxs[static_cast<std::size_t>(res)], Clock::now() < itp};
                }
                // timeout
                return {static_cast<int>(I0), false};
            }(tp, lks, Seqs{});
        }...}};

    // try rotations in seqs while there is time to retry
    result ret{};
    while ((ret = seqs[static_cast<std::size_t>(ret.idx)](end_time, locks)).retry) {
        std::this_thread::yield();
    }
    return ret.idx;
}
} // namespace detail

template <class Clock, class Duration, detail::TimedLockable... Ls>
[[nodiscard]] int try_lock_until(const std::chrono::time_point<Clock, Duration>& tp, Ls&... ls) {
    if constexpr (sizeof...(Ls) == 0) {
        return -1;
    } else if constexpr (sizeof...(Ls) == 1) {
        return -static_cast<int>(std::get<0>(std::tie(ls...)).try_lock_until(tp));
    } else {
        return detail::try_lock_until_impl(
            tp, std::tie(ls...), detail::make_pack_of_rotating_index_sequences<sizeof...(Ls)>{});
    }
}

template <class Rep, class Period, detail::TimedLockable... Ls>
[[nodiscard]] int try_lock_for(const std::chrono::duration<Rep, Period>& dur, Ls&... ls) {
    return try_lock_until(std::chrono::steady_clock::now() + dur, ls...);
}

template <detail::BasicLockable... Ms>
class multi_lock {
  public:
    using mutex_type = std::tuple<Ms*...>;

    // Constructors
    multi_lock() noexcept = default;

    explicit multi_lock(Ms&... ms)
        requires(sizeof...(Ms) > 0)
        : m_ms(std::addressof(ms)...) {
        lock();
    }

    multi_lock(std::defer_lock_t, Ms&... ms) noexcept : m_ms(std::addressof(ms)...) {}

    multi_lock(std::try_to_lock_t, Ms&... ms)
        requires(... && detail::Lockable<Ms>)
        : m_ms(std::addressof(ms)...) {
        try_lock();
    }

    multi_lock(std::adopt_lock_t, Ms&... ms) noexcept : m_ms(std::addressof(ms)...), m_locked(true) {}

    template <class Rep, class Period>
        requires(... && detail::TimedLockable<Ms>)
    multi_lock(const std::chrono::duration<Rep, Period>& dur, Ms&... ms) : m_ms(std::addressof(ms)...) {
        try_lock_for(dur);
    }

    template <class Clock, class Duration>
        requires(... && detail::TimedLockable<Ms>)
    multi_lock(const std::chrono::time_point<Clock, Duration>& tp, Ms&... ms) : m_ms(std::addressof(ms)...) {
        try_lock_until(tp);
    }

    // Destructor
    ~multi_lock() {
        if (m_locked)
            unlock();
    }

    // Move operations
    multi_lock(multi_lock&& other) noexcept
        : m_ms(std::exchange(other.m_ms, std::tuple<Ms*...>{})), m_locked(std::exchange(other.m_locked, false)) {}

    multi_lock& operator=(multi_lock&& other) noexcept {
        multi_lock(std::move(other)).swap(*this);
        return *this;
    }

    // Deleted copy operations
    multi_lock(const multi_lock&)            = delete;
    multi_lock& operator=(const multi_lock&) = delete;

    // Locking operations
  private:
    void lock_check() {
        if (m_locked) {
            throw std::system_error(std::make_error_code(std::errc::resource_deadlock_would_occur));
        }
        if constexpr (sizeof...(Ms) != 0) {
            if (std::get<0>(m_ms) == nullptr) {
                throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
            }
        }
    }

  public:
    void lock()
        requires(sizeof...(Ms) == 1 || (... && detail::Lockable<Ms>))
    {
        lock_check();
        if constexpr (sizeof...(Ms) == 1) {
            std::get<sizeof...(Ms) - 1>(m_ms)->lock();
        } else if constexpr (sizeof...(Ms) > 1) {
            std::apply([](auto... ms) { std::lock(*ms...); }, m_ms);
        }
        m_locked = true;
    }

    int try_lock()
        requires(... && detail::Lockable<Ms>)
    {
        lock_check();
        int rv;
        if constexpr (sizeof...(Ms) == 0) {
            rv = -1;
        } else if constexpr (sizeof...(Ms) == 1) {
            rv = -static_cast<int>(std::get<sizeof...(Ms) - 1>(m_ms)->try_lock());
        } else {
            rv = std::apply([](auto... ms) { return std::try_lock(*ms...); }, m_ms);
        }
        m_locked = rv == -1;
        return rv;
    }

    template <class Rep, class Period>
        requires(... && detail::TimedLockable<Ms>)
    int try_lock_for(const std::chrono::duration<Rep, Period>& dur) {
        lock_check();
        int rv   = std::apply([&](auto... ms) { return beman::timed_lock_alg::try_lock_for(dur, *ms...); }, m_ms);
        m_locked = rv == -1;
        return rv;
    }

    template <class Clock, class Duration>
        requires(... && detail::TimedLockable<Ms>)
    int try_lock_until(const std::chrono::time_point<Clock, Duration>& tp) {
        lock_check();
        int rv   = std::apply([&](auto... ms) { return beman::timed_lock_alg::try_lock_until(tp, *ms...); }, m_ms);
        m_locked = rv == -1;
        return rv;
    }

    void unlock() {
        if (not m_locked) {
            throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
        }
        // clang doesn't seem to understand that "unlocker" is actually used to unlock all mutexes at the end of the
        // scope even if one of them throws so mark it as maybe_unused.
        [[maybe_unused]] auto unlocker =
            std::apply([](auto... ms) { return std::scoped_lock(std::adopt_lock, *ms...); }, m_ms);
        m_locked = false;
    }

    // Modifiers
    void swap(multi_lock& other) noexcept {
        std::swap(m_ms, other.m_ms);
        std::swap(m_locked, other.m_locked);
    }

    mutex_type release() noexcept {
        m_locked = false;
        return std::exchange(m_ms, mutex_type{});
    }

    // Observers
    mutex_type mutex() const noexcept { return m_ms; }
    bool       owns_lock() const noexcept { return m_locked; }
    explicit   operator bool() const noexcept { return m_locked; }

  private:
    mutex_type m_ms;
    bool       m_locked = false;
};

template <class... Ms>
void swap(multi_lock<Ms...>& lhs, multi_lock<Ms...>& rhs) noexcept {
    lhs.swap(rhs);
}
} // namespace beman::timed_lock_alg
#endif
