#include "timer.h"
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <ostream>

Timer::Timer(std::string&& prefix, std::ostream& out)
    : prefix_(std::move(prefix)), out_(out) {}

Timer::~Timer() {}

void Timer::run(std::function<void()> f) {
    auto begin = std::chrono::steady_clock::now();
    f();
    auto end = std::chrono::steady_clock::now();
    out_ << prefix_ << " " << std::fixed << std::setprecision(2)
         << std::chrono::duration_cast<fsecs>(end - begin).count() << std::endl;
}
