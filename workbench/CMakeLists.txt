#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Balan Narcis (balannarcis96@gmail.com)
#
cmake_minimum_required(VERSION 4.0.0)
project(skl-config-workbench LANGUAGES CXX VERSION 1.0.0)

file(GLOB_RECURSE _SKL_CONFIG_WORKBENCH_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/**/*.cpp")

add_executable(${PROJECT_NAME} ${_SKL_CONFIG_WORKBENCH_SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/source")

# Link Skylake Config lib
target_link_libraries(${PROJECT_NAME} PUBLIC libskl-config)
