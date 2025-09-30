/*
 * A simple example with 30 timed mutexes where one is locked
 * for 40 milliseconds before being released.
 */

#include <beman/timed_lock_alg/mutex.hpp>

#include <chrono>
#include <iostream>
#include <thread>

namespace tla = beman::timed_lock_alg;

constexpr std::chrono::milliseconds yield(10);

void foo(std::timed_mutex& m) {
    std::lock_guard lock(m);
    std::this_thread::sleep_for(std::chrono::milliseconds(40) + yield);
}

int main() {
    std::cout << std::boolalpha;
    std::array<std::timed_mutex, 30> mtxs;

    for (int ms = 10; ms <= 70; ms += 20) { // try 10, 30, 50, 70 ms
        std::chrono::milliseconds cms(ms);
        // start a thread that locks the last mutex unless mtxs is empty
        std::jthread jt = [&]() -> std::jthread {
            if constexpr (not mtxs.empty()) {
                return std::jthread(foo, std::ref(mtxs.back()));
            } else {
                return {};
            }
        }();
        std::this_thread::sleep_for(yield);

        std::cout << "trying for " << cms << '\n';

        auto start = std::chrono::steady_clock::now();
        auto r1    = std::apply([&](auto&... mxs) { return tla::try_lock_for(cms, mxs...); }, mtxs);
        auto end   = std::chrono::steady_clock::now();

        // should be done in approx. 10, 30, 40 and 40 ms, where the two last tries succeeds:
        std::cout << "done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << ": ";

        if (r1 == -1) {
            auto sl = std::apply([&](auto&... mxs) { return std::scoped_lock(std::adopt_lock, mxs...); }, mtxs);
            std::cout << "got lock";

        } else {
            std::cout << "failed on lockable " << r1;
        }
        std::cout << "\n\n";
    }
}
