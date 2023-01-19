#include "batcher_sort.h"
#include "defs.h"
#include "shaders.h"
#include "timer.h"
#include "vk_util.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>

void sort_vec(VkInfo* info) {
    // Initialize sort shader */
    create_shader(MERGE_SHADER, MERGE_SHADER_LEN, info);

    // Start queuing the sequence of commands
    // to be executed on GPU
    begin_command_buffer(info);

    // Transfer the array to GPU
    load_input(info);
    put_write_read_barrier(Transfer, Shader, info);

    // Get the size of the array
    uint32_t n = info->arr.get_elements_num();

    // Set the upper power of 2 as an imaginative size
    // (real bounds are checked inside the shader)
    uint32_t N = 1;
    while (N < n) {
        N *= 2;
    }

    uint32_t merge_group_size = 2;
    while (merge_group_size <= N) {
        uint32_t inner_rem = 0;
        for (uint32_t stride = merge_group_size >> 1; stride >= 1;
             stride >>= 1) {
            uint32_t stride_trailing_zeros = __builtin_ctz(stride);
            uint32_t inner_last_idx =
                (merge_group_size >> stride_trailing_zeros) - 1;
            std::vector<push_cst_t> push_csts = {n,                     //
                                                 stride,                //
                                                 stride_trailing_zeros, //
                                                 inner_rem,             //
                                                 inner_last_idx};
            bind_constants(push_csts, info);
            // Queue a merge layer
            dispatch(n, TILE_SIZE, info);
            put_write_read_barrier(Shader, Shader, info);

            // Starting from the second iteration, inner index
            // should be odd to be the left one
            inner_rem = 1;
        }
        merge_group_size <<= 1;
    }
    put_write_read_barrier(Shader, Transfer, info);

    // Transfer array back grom GPU
    load_output(info);

    // Mark the end of the buffer,
    end_command_buffer(info);
    // Submit it and wait until it's computed
    submit(info);
}
