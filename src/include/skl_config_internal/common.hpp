//!
//! \file common
//!
//! \license Licensed under the MIT License. See LICENSE for details.
//!
#pragma once

#include <string>

#include <skl_int>
#include <skl_buffer_view>

namespace skl::config {
template <typename _Type>
concept CConfigTargetType = __is_class(_Type);

template <typename _Field>
concept CIntegerValueFieldType = __is_same(_Field, i8)
                              || __is_same(_Field, u8)
                              || __is_same(_Field, i16)
                              || __is_same(_Field, u16)
                              || __is_same(_Field, i32)
                              || __is_same(_Field, u32)
                              || __is_same(_Field, i64)
                              || __is_same(_Field, u64);

template <typename _Field>
concept CNumericValueFieldType = CIntegerValueFieldType<_Field>
                              || __is_same(_Field, float)
                              || __is_same(_Field, double);

template <typename _Field>
concept CStringValueFieldType = __is_same(_Field, std::string)
                             || __is_same(_Field, skl_buffer_view);

template <typename _Field>
concept CValueFieldType = CNumericValueFieldType<_Field>
                       || CStringValueFieldType<_Field>;
} // namespace skl::config
