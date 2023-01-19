#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <vector>
#include <vulkan/vulkan.h>

template <typename T> class Array {
  public:
    using element_type = T;
    Array(uint32_t n) : n(n), buf_size(sizeof(element_type) * n) {}
    Array() {}
    void fill_random(uint32_t seed) {
        std::mt19937 gen(seed);
        if constexpr (std::is_integral<T>::value) {
            std::uniform_int_distribution<T> dis( //
                0, std::numeric_limits<T>::max());
            std::generate(buf, buf + n, [&]() { return dis(gen); });

        } else {
            std::uniform_real_distribution<T> dis(
                0, std::numeric_limits<T>::max());
            std::generate(buf, buf + n, [&]() { return dis(gen); });
        }
    }
    void debug_print(uint32_t n) {
        std::cout << "---- ARRAY BEGIN ---" << std::endl;
        for (uint32_t i = 0; i < n; i++) {
            std::cout << buf[i] << ' ';
        }
        std::cout << std::endl;
        std::cout << "----  ARRAY END  ---" << std::endl;
    }
    bool compare_with_reference(const std::vector<T>& ref) {
        for (uint32_t i = 0; i < n; i++) {
            if (buf[i] != ref[i]) {
                return false;
            }
        }
        return true;
    }
    element_type*& get_buffer() { return buf; }
    uint32_t get_elements_num() { return n; }
    VkDeviceSize get_buffer_size() { return buf_size; }
    VkBuffer& get_host_buffer() { return host_buffer; }
    const VkBuffer& get_host_buffer() const { return host_buffer; }
    VkBuffer& get_device_buffer() { return device_buffer; }
    const VkBuffer& get_device_buffer() const { return device_buffer; }
    VkDeviceMemory& get_host_memory() { return host_memory; }
    const VkDeviceMemory& get_host_memory() const { return host_memory; }
    VkDeviceMemory& get_device_memory() { return device_memory; }
    const VkDeviceMemory& get_device_memory() const { return device_memory; }
    ~Array() {}

  private:
    uint32_t n;
    element_type* buf;
    size_t buf_size;
    VkBuffer host_buffer;
    VkBuffer device_buffer;
    VkDeviceMemory host_memory;
    VkDeviceMemory device_memory;
};
