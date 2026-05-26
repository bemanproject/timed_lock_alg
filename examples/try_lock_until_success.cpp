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

void foo(std::timed_mutex& m0, std::timed_mutex& m1) {
    std::lock(m0, m1);
    std::cout << "locked\n";
    std::this_thread::sleep_for(200ms);
    m0.unlock();
    std::cout << "0 unlocked\n";
    std::this_thread::sleep_for(100ms);
    m1.unlock();
    std::cout << "1 unlocked\n";
}

int main() {
    const char* name[]{"foo", "bar"};
    const char* act[]{"ping", "pong"};

    std::timed_mutex m0, m1;
    JThread          th(foo, std::ref(m0), std::ref(m1));
    std::this_thread::sleep_for(100ms);

    std::cout << "trying\n";
    for (int rv; (rv = tla::try_lock_for(20ms, m0, m1)) != -1;) {
        std::cout << "failed on " << name[rv] << ", taking action " << act[rv] << '\n';
    }
    std::cout << "success\n";
}
