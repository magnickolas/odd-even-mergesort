#pragma once

#include "defs.h"
#include "vk_array.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

using push_cst_t = uint32_t;

enum MemoryAccessType { Transfer, Shader };

[[maybe_unused]] static std::string err_string(VkResult err_code) {
    switch (err_code) {
#define STR(r)                                                                 \
    case VK_##r:                                                               \
        return #r
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_INVALID_SHADER_NV);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(EVENT_RESET);
        STR(EVENT_SET);
        STR(INCOMPLETE);
        STR(NOT_READY);
        STR(SUBOPTIMAL_KHR);
        STR(TIMEOUT);
#undef STR
    default:
        return "UNKNOWN_ERROR";
    }
}

#define VK_CHECK_RESULT(f)                                                     \
    {                                                                          \
        VkResult res = (f);                                                    \
        if (res != VK_SUCCESS) {                                               \
            std::cerr << "Fatal : VkResult is \"" << err_string(res)           \
                      << "\" in " << __FILE__ << " at line " << __LINE__       \
                      << "\n";                                                 \
            assert(res == VK_SUCCESS);                                         \
        }                                                                      \
    }

struct VkInfo {
    Array<CTYPE> arr;
    VkCommandBuffer command_buffer;
    VkCommandPool command_pool;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDevice device;
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkQueue queue;
    VkShaderModule shader_module;
    uint32_t queue_family_index;
};

void set_instance(VkInfo* vk_info);

void set_physical_device(VkInfo* vk_info);

void create_shader(const unsigned char* data, size_t size, VkInfo* vk_info);

void create_shader_from_file(std::string name, VkInfo* vk_info);

uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties,
                          VkInfo* vk_info);

void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkBuffer& buffer,
                   VkDeviceMemory& bufferMemory, VkInfo* vk_info);

void load_input(VkInfo* vk_info);

void load_output(VkInfo* vk_info);

void begin_command_buffer(VkInfo* vk_info);

void end_command_buffer(VkInfo* vk_info);

void bind_constants(const std::vector<push_cst_t>& push_csts, VkInfo* vk_info);

void dispatch(uint32_t n, uint32_t tile_size, VkInfo* vk_info);

void submit(VkInfo* vk_info);

void destroy(VkInfo* vk_info);

void put_write_read_barrier(MemoryAccessType m_src, MemoryAccessType m_dst,
                            VkInfo* vk_info);

Array<CTYPE> create_array_storage(uint32_t n, VkInfo* vk_info);

void destroy_array_storage(const Array<CTYPE>& arr, VkInfo* vk_info);

struct VkInfoGuard {
    VkInfo info;
    VkInfoGuard(uint32_t n, uint32_t seed);
    VkInfo* get();
    ~VkInfoGuard();
};
