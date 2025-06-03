#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Balan Narcis (balannarcis96@gmail.com)
#
include_guard()

include(FetchContent)

if(NOT TARGET nlohmann_json)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

if(PROJECT_IS_TOP_LEVEL)
    FetchContent_Declare(
        skylake-core
        GIT_REPOSITORY https://github.com/balannarcis96/skylake-core.git
    )
    FetchContent_MakeAvailable(skylake-core)

    # Private SkylakeCore library
    set(SKL_CONFIG_SKL_CORE_TARGET "libskl-core-cfg")
    skl_CreateSkylakeCoreLibTarget("${SKL_CONFIG_SKL_CORE_TARGET}" "")
endif()

if(NOT SKL_CONFIG_SKL_CORE_TARGET OR NOT TARGET ${SKL_CONFIG_SKL_CORE_TARGET})
    message(FATAL_ERROR "SkylakeConfig requires SKL_CONFIG_SKL_CORE_TARGET to be defined and contain the name of a SkylakeCore target instance")
endif()
