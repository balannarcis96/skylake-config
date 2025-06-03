#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Balan Narcis (balannarcis96@gmail.com)
#
cmake_minimum_required(VERSION 4.0.0)
project(libskl-config LANGUAGES CXX VERSION 1.0.0)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
    INTERFACE SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Link Skylake Core lib
target_link_libraries(${PROJECT_NAME} INTERFACE ${SKL_CONFIG_SKL_CORE_TARGET} nlohmann_json)

# Remove default prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
