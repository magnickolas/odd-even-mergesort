#include "vk_util.h"
#include "defs.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

static const std::vector<const char*> validation_layers = {
#if DEBUG
    "VK_LAYER_KHRONOS_validation"
#endif
};

#if DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageType,
               const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
               void* pUserData) {
    (void)messageSeverity;
    (void)messageType;
    (void)pUserData;
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}
#endif

static bool check_validation_layer_support() {
    uint32_t nlayers;
    vkEnumerateInstanceLayerProperties(&nlayers, NULL);
    std::vector<VkLayerProperties> available_layers(nlayers);
    vkEnumerateInstanceLayerProperties(&nlayers, available_layers.data());
    for (const char* layerName : validation_layers) {
        bool found_layer = false;
        for (const auto& layer_props : available_layers) {
            if (strcmp(layerName, layer_props.layerName) == 0) {
                found_layer = true;
                break;
            }
        }
        if (!found_layer) {
            return false;
        }
    }
    return true;
}

void set_instance(VkInfo* vk_info) {
    if (!check_validation_layer_support()) {
        throw std::runtime_error(
            "validation layers requested, but not available!");
    }
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "EntryTask";

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(validation_layers.size()),
        .ppEnabledLayerNames = validation_layers.data(),
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
    };
#if DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = NULL,
        .flags = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData = NULL,
    };
    create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
#endif

    VK_CHECK_RESULT(vkCreateInstance(&create_info, NULL, &vk_info->instance));
}

void set_physical_device(VkInfo* vk_info) {
    uint32_t gpu_count;
    VK_CHECK_RESULT(
        vkEnumeratePhysicalDevices(vk_info->instance, &gpu_count, NULL));
    assert(gpu_count >= 1);
    std::vector<VkPhysicalDevice> devices(gpu_count);
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vk_info->instance, &gpu_count,
                                               devices.data()));
    vk_info->physical_device = devices[0];

    vkGetPhysicalDeviceMemoryProperties(vk_info->physical_device,
                                        &vk_info->memory_properties);

    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = 0, // TO BE CHANGED
        .queueCount = 16,
        .pQueuePriorities = &queue_priority,
    };

    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_info->physical_device,
                                             &queue_family_count, NULL);
    assert(queue_family_count >= 1);

    std::vector<VkQueueFamilyProperties> queue_props(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        vk_info->physical_device, &queue_family_count, queue_props.data());
    assert(queue_family_count >= 1);

    bool found = false;
    for (unsigned int i = 0; i < queue_family_count; i++) {
        if ((queue_props[i].queueFlags &
             (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)) ==
            (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)) {
            queue_info.queueFamilyIndex = i;
            found = true;
        }
    }
    assert(found);
    assert(queue_family_count >= 1);
    vk_info->queue_family_index = queue_info.queueFamilyIndex;

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .pEnabledFeatures = NULL,
    };
    VK_CHECK_RESULT(vkCreateDevice(vk_info->physical_device, &device_info, NULL,
                                   &vk_info->device));
    vkGetDeviceQueue(vk_info->device, vk_info->queue_family_index, 0,
                     &vk_info->queue);
}

void create_shader_from_file(std::string name, VkInfo* vk_info) {
    std::ifstream spirvfile(name.c_str(), std::ios::binary | std::ios::ate);
    std::streampos spirvsize = spirvfile.tellg();
    assert(spirvsize > 0);
    spirvfile.seekg(0, std::ios::beg);

    unsigned char* spirv = new unsigned char[spirvsize];
    spirvfile.read(reinterpret_cast<char*>(spirv), spirvsize);
    create_shader(spirv, spirvsize, vk_info);
}

void create_shader(const unsigned char* spirv, size_t size, VkInfo* vk_info) {
    VkShaderModuleCreateInfo shader_module_create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = size,
        .pCode = reinterpret_cast<const uint32_t*>(spirv),
    };

    VK_CHECK_RESULT(vkCreateShaderModule(vk_info->device,
                                         &shader_module_create_info, NULL,
                                         &vk_info->shader_module));

    VkDescriptorSetLayoutBinding layout_binding;
    layout_binding = {.binding = 0,
                      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                      .descriptorCount = 1,
                      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                      .pImmutableSamplers = 0};

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &layout_binding,
    };

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(
        vk_info->device, &descriptor_set_layout_create_info, NULL,
        &vk_info->descriptor_set_layout));

    VkPushConstantRange ranges[] = {
        {VK_SHADER_STAGE_COMPUTE_BIT, 0, NUM_PUSH_CSTS * sizeof(push_cst_t)},
    };

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &vk_info->descriptor_set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = ranges,
    };

    VK_CHECK_RESULT(vkCreatePipelineLayout(vk_info->device,
                                           &pipeline_layout_create_info, NULL,
                                           &vk_info->pipeline_layout));

    VkDescriptorPoolSize poolSize = {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                     .descriptorCount = 1};

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,
    };

    VK_CHECK_RESULT(vkCreateDescriptorPool(vk_info->device,
                                           &descriptor_pool_create_info, NULL,
                                           &vk_info->descriptor_pool));

    VkDescriptorSetAllocateInfo set_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = vk_info->descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &vk_info->descriptor_set_layout,
    };

    VK_CHECK_RESULT(vkAllocateDescriptorSets(
        vk_info->device, &set_allocate_info, &vk_info->descriptor_set));

    VkDescriptorBufferInfo arr_info = {
        .buffer = vk_info->arr.get_device_buffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };

    VkWriteDescriptorSet write_descriptor_set = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = 0,
        .dstSet = vk_info->descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = 0,
        .pBufferInfo = &arr_info,
        .pTexelBufferView = 0};

    vkUpdateDescriptorSets(vk_info->device, 1, &write_descriptor_set, 0, 0);

    VkCommandPoolCreateInfo command_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vk_info->queue_family_index,
    };

    VK_CHECK_RESULT(vkCreateCommandPool(vk_info->device,
                                        &command_pool_create_info, NULL,
                                        &vk_info->command_pool));

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = vk_info->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VK_CHECK_RESULT(vkAllocateCommandBuffers(vk_info->device,
                                             &command_buffer_allocate_info,
                                             &vk_info->command_buffer));

    VkPipelineShaderStageCreateInfo shader_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = vk_info->shader_module,
        .pName = "main",
        .pSpecializationInfo = NULL,
    };

    VkComputePipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stage = shader_create_info,
        .layout = vk_info->pipeline_layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
    };

    VK_CHECK_RESULT(vkCreateComputePipelines(vk_info->device, VK_NULL_HANDLE, 1,
                                             &pipeline_create_info, NULL,
                                             &vk_info->pipeline));
}

uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties,
                          VkInfo* vk_info) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vk_info->physical_device,
                                        &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkBuffer& buffer,
                   VkDeviceMemory& bufferMemory, VkInfo* vk_info) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vk_info->device, &bufferInfo, nullptr, &buffer) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vk_info->device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        find_memory_type(memRequirements.memoryTypeBits, properties, vk_info);

    if (vkAllocateMemory(vk_info->device, &allocInfo, nullptr, &bufferMemory) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(vk_info->device, buffer, bufferMemory, 0);
}

void begin_command_buffer(VkInfo* vk_info) {
    VkCommandBufferBeginInfo command_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = NULL,
    };
    vkBeginCommandBuffer(vk_info->command_buffer, &command_buffer_begin_info);
}

void end_command_buffer(VkInfo* vk_info) {
    vkEndCommandBuffer(vk_info->command_buffer);
}

void bind_constants(const std::vector<push_cst_t>& push_csts, VkInfo* vk_info) {
    vkCmdPushConstants(vk_info->command_buffer, vk_info->pipeline_layout,
                       VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       push_csts.size() * sizeof(push_cst_t), push_csts.data());
}

void dispatch(uint32_t n, uint32_t tile_size, VkInfo* vk_info) {
    n = (n + tile_size - 1) - (n - 1) % tile_size;
    assert(n % tile_size == 0);
    vkCmdBindPipeline(vk_info->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                      vk_info->pipeline);
    vkCmdBindDescriptorSets(
        vk_info->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
        vk_info->pipeline_layout, 0, 1, &vk_info->descriptor_set, 0, 0);
    vkCmdDispatch(vk_info->command_buffer, n / tile_size, 1, 1);
}

void submit(VkInfo* vk_info) {
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = NULL,
        .pWaitDstStageMask = NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = &vk_info->command_buffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = NULL,
    };

    VK_CHECK_RESULT(
        vkQueueSubmit(vk_info->queue, 1, &submit_info, VK_NULL_HANDLE));
    VK_CHECK_RESULT(vkQueueWaitIdle(vk_info->queue));
}

void destroy(VkInfo* vk_info) {
    vkFreeCommandBuffers(vk_info->device, vk_info->command_pool, 1,
                         &vk_info->command_buffer);
    vkDestroyCommandPool(vk_info->device, vk_info->command_pool, NULL);
    vkDestroyDescriptorPool(vk_info->device, vk_info->descriptor_pool, NULL);
    vkDestroyPipeline(vk_info->device, vk_info->pipeline, NULL);
    vkDestroyPipelineLayout(vk_info->device, vk_info->pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(vk_info->device,
                                 vk_info->descriptor_set_layout, NULL);
    vkDestroyShaderModule(vk_info->device, vk_info->shader_module, NULL);
    destroy_array_storage(vk_info->arr, vk_info);
    vkDestroyDevice(vk_info->device, NULL);
    vkDestroyInstance(vk_info->instance, NULL);
}

void load_input(VkInfo* vk_info) {
    VkBufferCopy buffer_copy = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = vk_info->arr.get_buffer_size(),
    };
    vkCmdCopyBuffer(vk_info->command_buffer, vk_info->arr.get_host_buffer(),
                    vk_info->arr.get_device_buffer(), 1, &buffer_copy);
}

void load_output(VkInfo* vk_info) {
    VkBufferCopy buffer_copy = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = vk_info->arr.get_buffer_size(),
    };
    vkCmdCopyBuffer(vk_info->command_buffer, vk_info->arr.get_device_buffer(),
                    vk_info->arr.get_host_buffer(), 1, &buffer_copy);
}

void put_write_read_barrier(MemoryAccessType m_src, MemoryAccessType m_dst,
                            VkInfo* vk_info) {
    VkMemoryBarrier mb = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = m_src == Transfer ? VK_ACCESS_TRANSFER_WRITE_BIT
                                           : VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = m_dst == Transfer ? VK_ACCESS_TRANSFER_READ_BIT
                                           : VK_ACCESS_SHADER_READ_BIT,
    };
    vkCmdPipelineBarrier(
        vk_info->command_buffer,
        m_src == Transfer ? VK_PIPELINE_STAGE_TRANSFER_BIT
                          : VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        m_dst == Transfer ? VK_PIPELINE_STAGE_TRANSFER_BIT
                          : VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 1, &mb, 0, 0, 0, 0);
}

Array<CTYPE> create_array_storage(uint32_t n, VkInfo* vk_info) {
    Array<CTYPE> arr{n};

    create_buffer(arr.get_buffer_size(),
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  arr.get_host_buffer(), arr.get_host_memory(), vk_info);

    create_buffer(arr.get_buffer_size(),
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, arr.get_device_buffer(),
                  arr.get_device_memory(), vk_info);

    vkMapMemory(vk_info->device, arr.get_host_memory(), 0,
                arr.get_buffer_size(), 0, (void**)&arr.get_buffer());

    return arr;
}

void destroy_array_storage(const Array<CTYPE>& arr, VkInfo* vk_info) {
    vkUnmapMemory(vk_info->device, arr.get_host_memory());
    vkDestroyBuffer(vk_info->device, arr.get_host_buffer(), NULL);
    vkFreeMemory(vk_info->device, arr.get_host_memory(), NULL);
    vkDestroyBuffer(vk_info->device, arr.get_device_buffer(), NULL);
    vkFreeMemory(vk_info->device, arr.get_device_memory(), NULL);
}

VkInfoGuard::VkInfoGuard() {
    set_instance(&info);
    set_physical_device(&info);
}
VkInfo* VkInfoGuard::get() { return &info; }
VkInfoGuard::~VkInfoGuard() { destroy(&info); }
