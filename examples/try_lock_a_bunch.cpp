// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

/*
 * A simple example with 30 timed mutexes where one is locked
 * for 40 milliseconds before being released.
 */

#include <beman/timed_lock_alg/mutex.hpp>

#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <utility>

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
} // namespace

using namespace std::chrono_literals;
namespace tla = beman::timed_lock_alg;

constexpr auto yield = 10ms;

void foo(std::timed_mutex& m) {
    std::lock_guard lock(m);
    std::this_thread::sleep_for(40ms + yield);
}

int main() {
    std::cout << std::boolalpha;
    std::array<std::timed_mutex, 30> mtxs;

    for (auto ms = 10ms; ms <= 70ms; ms += 20ms) { // try 10, 30, 50, 70 ms
        // start a thread that locks the last mutex unless mtxs is empty
        JThread jt = [&]() -> JThread {
            if constexpr (not mtxs.empty()) {
                return JThread(foo, std::ref(mtxs.back()));
            } else {
                return {};
            }
        }();
        std::this_thread::sleep_for(yield);

        std::cout << "trying for " << ms.count() << "ms\n";

        auto start = std::chrono::steady_clock::now();
        auto r1    = std::apply([&](auto&... mxs) { return tla::try_lock_for(ms, mxs...); }, mtxs);
        auto end   = std::chrono::steady_clock::now();

        // should be done in approx. 10, 30, 40 and 40 ms, where the two last tries succeeds:
        std::cout << "done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                  << "ms: ";

        if (r1 == -1) {
            auto sl = std::apply([&](auto&... mxs) { return std::scoped_lock(std::adopt_lock, mxs...); }, mtxs);
            std::cout << "got lock";

        } else {
            std::cout << "failed on lockable " << r1;
        }
        std::cout << "\n\n";
    }
}
