#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Balan Narcis (balannarcis96@gmail.com)
#
cmake_minimum_required(VERSION 4.0.0)
project(libskl-config LANGUAGES CXX VERSION 1.0.0)

file(GLOB_RECURSE _SKL_CONFIG_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/**/*.cpp")

add_library(${PROJECT_NAME} STATIC ${_SKL_CONFIG_SOURCE_FILES})

target_include_directories(${PROJECT_NAME}
    PUBLIC SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/source")

# Link Skylake Core lib
target_link_libraries(${PROJECT_NAME} PUBLIC libskl-core-cfg nlohmann_json)

# Remove default prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

# Tuning
skl_add_tune_header_to_target(${PROJECT_NAME} "skl_config" DEFAULT_PRESET_FILE "${SkylakeConfig_SOURCE_DIR}/cmake/presets/default_presets.json")
