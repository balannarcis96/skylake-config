#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Balan Narcis (balannarcis96@gmail.com)
#
cmake_minimum_required(VERSION 4.0.0)

add_subdirectory(skylake-core)

if(NOT TARGET nlohmann_json)
    add_subdirectory(json)
endif()
