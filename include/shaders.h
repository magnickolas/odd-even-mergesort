#pragma once
#include <cstddef>

constexpr unsigned char MERGE_SHADER[] = {
#include "shaders/merge_dump.h"
};

constexpr size_t MERGE_SHADER_LEN = sizeof(MERGE_SHADER);
