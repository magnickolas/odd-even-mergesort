add_executable(batcher_sort
  batcher_sort.cc vk_util.cc timer.cc opts.cc main.cc)
set_target_properties(batcher_sort PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
target_include_directories(batcher_sort PRIVATE
  ../include)
target_compile_features(
  batcher_sort PRIVATE
  cxx_std_20)
target_compile_options(
  batcher_sort PRIVATE
  -Wall -Wextra -pedantic-errors -O2)

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIR})
target_link_libraries(batcher_sort ${Vulkan_LIBRARY})

add_dependencies(batcher_sort merge-shader)
target_include_directories(batcher_sort PRIVATE
  ${CMAKE_BINARY_DIR})
