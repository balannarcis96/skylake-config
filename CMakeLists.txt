#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Balan Narcis (balannarcis96@gmail.com)
#
cmake_minimum_required(VERSION 4.0.0)
project(SkylakeConfig LANGUAGES CXX VERSION 1.0.0)

# Update modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set(SKL_CONFIG_ENABLE_TESTS ON CACHE BOOL "[TopLevel] Enable tests")

if(NOT PROJECT_IS_TOP_LEVEL)
    set(SKL_CONFIG_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
endif()

include(SkylakeConfigDeps)

add_subdirectory(src)

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(workbench)

    if(SKL_CONFIG_ENABLE_TESTS)
        # Enable CTest
        enable_testing()
        
        # Add test module
        include(SkylakeConfigTest)

        # Add tests
        add_subdirectory(test)
    endif()
endif()
