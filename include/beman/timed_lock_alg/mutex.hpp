#ifndef TRY_LOCK_FOR_UNTIL_HPP_6FE470F4_744E_11F0_BE3A_90B11C0C0FF8
#define TRY_LOCK_FOR_UNTIL_HPP_6FE470F4_744E_11F0_BE3A_90B11C0C0FF8

#include <array>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <thread>
#include <tuple>
#include <utility>

#include <chrono>
#include <concepts>

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
template <class Timepoint, class Locks, class... Seqs>
int try_lock_until_impl(const Timepoint& end_time, Locks locks, type_pack<Seqs...>) {
    // an array with one function per lockable/sequence to try:
    std::array<result (*)(const Timepoint&, Locks&), sizeof...(Seqs)> seqs{{+[](const Timepoint& tp, Locks& lks) {
        return []<class Clock, class Duration, std::size_t I0, std::size_t... Is>(
                   const std::chrono::time_point<Clock, Duration>& itp,
                   Locks&                                          lcks,
                   std::index_sequence<I0, Is...>) -> result {
            if (std::unique_lock first{std::get<I0>(lcks), itp}) {
                int res = friendly_try_lock(std::get<Is>(lcks)...);
                if (res == -1) {
                    first.release();
                    return {-1, false}; // success
                }
                // return the index to try_lock_until next round
                return {static_cast<int>(std::array{Is...}[static_cast<std::size_t>(res)]), true};
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
} // namespace beman::timed_lock_alg
#endif
