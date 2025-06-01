//!
//! \file common
//!
//! \license Licensed under the MIT License. See LICENSE for details.
//!
#pragma once

#include <string>

#include <skl_int>

namespace skl::config {
template <typename _Type>
concept CConfigTargetType = __is_class(_Type);

template <typename _Type>
concept CVectorType = requires(_Type f_vec, typename _Type::value_type f_val) {
    { f_vec.begin() };
    { f_vec.end() };
    { f_vec.push_back(f_val) };
};

template <typename _Field>
concept CIntegerValueFieldType = __is_same(_Field, i8)
                       || __is_same(_Field, u8)
                       || __is_same(_Field, i16)
                       || __is_same(_Field, u16)
                       || __is_same(_Field, i32)
                       || __is_same(_Field, u32)
                       || __is_same(_Field, i64)
                       || __is_same(_Field, u64)
                       || __is_same(_Field, float)
                       || __is_same(_Field, double);

template <typename _Field>
concept CValueFieldType = __is_same(_Field, i8)
                       || __is_same(_Field, u8)
                       || __is_same(_Field, i16)
                       || __is_same(_Field, u16)
                       || __is_same(_Field, i32)
                       || __is_same(_Field, u32)
                       || __is_same(_Field, i64)
                       || __is_same(_Field, u64)
                       || __is_same(_Field, float)
                       || __is_same(_Field, double)
                       || __is_same(_Field, std::string);
} // namespace skl::config
