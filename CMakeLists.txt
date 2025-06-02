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
    set(SKL_CORE_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
else()
    set(SKL_CONFIG_PRIVATE_CORE_LIB ON)
endif()

add_subdirectory(third_party)

if(TARGET "libskl-core-cfg")
    message(FATAL_ERROR "SkylakeConfig requires \"libskl-core-cfg\" to not be defined")
endif()

# Private SkylakeCore library
skl_CreateSkylakeCoreLibTarget("libskl-core-cfg" "")

add_subdirectory(src)

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(workbench)

    if(SKL_CONFIG_ENABLE_TESTS)
        # Enable CTest
        enable_testing()

        include(SkylakeConfigTest)

        # Add the gtest library
        if(NOT TARGET gtest)
            set(BUILD_GMOCK OFF)
            set(INSTALL_GTEST OFF)
            add_subdirectory(third_party/skylake-core/third_party/googletest)
        endif()

        add_subdirectory(test)
    endif()
endif()
