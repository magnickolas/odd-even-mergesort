cmake_minimum_required(VERSION 3.17)

find_program(GLSLC glslc)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SHADER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})
set(shader-source ${SHADER_DIR}/merge.comp)
set(shader-spv ${SHADER_BINARY_DIR}/merge.spv)

set(GLSLC_ARGS
  -fshader-stage=compute
  -I
  ${SHADER_INCLUDE_DIR}
)

# Generate .spv with glslc
add_custom_command(
  OUTPUT ${shader-spv}
  COMMAND ${GLSLC} ${GLSLC_ARGS} -o ${shader-spv} ${shader-source}
  DEPENDS ${shader-source}
  COMMENT "Compiling shader ${shader-source}")
set_source_files_properties(${shader-spv} PROPERTIES GENERATED TRUE)

set(shader-embed ${SHADER_BINARY_DIR}/merge_dump.h)

# Embed generated .spv to .h file
add_custom_command(
  OUTPUT ${shader-embed}
  COMMAND xxd -i < ${shader-spv} > ${shader-embed}
  DEPENDS ${shader-spv}
  COMMENT "Embedding shader ${shader-spv} to ${shader-embed}")

add_custom_target(merge-shader DEPENDS ${shader-embed})
