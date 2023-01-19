#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <ostream>
#include <string>

class Timer {
    using fsecs = std::chrono::duration<double, std::chrono::seconds::period>;

  public:
    Timer(std::string&& prefix, std::ostream& = std::cout);
    ~Timer();
    void run(std::function<void()>);

  private:
    std::string prefix_;
    std::ostream& out_;
};
