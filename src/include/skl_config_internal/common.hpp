//!
//! \file common
//!
//! \license Licensed under the MIT License. See LICENSE for details.
//!
#pragma once

#include <string>

#include <skl_int>
#include <skl_buffer_view>
#include <skl_traits/same_as>

namespace skl::config {
template <typename _Type>
concept CConfigTargetType = __is_class(_Type);

class Field;

template <typename _TargetType, typename _ProxyType>
concept CConfigProxyType = __is_class(_ProxyType)
                        && requires(_TargetType f_target, _ProxyType& f_proxy, Field& f_field) {
                               { f_proxy.submit(f_field, f_target) } -> same_as<void>;
                           } && requires(const _TargetType& f_target, _ProxyType f_proxy, Field& f_field) {
                               { f_proxy.load(f_field, f_target) } -> same_as<bool>;
                           };

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
concept CBooleanValueFieldType = CIntegerValueFieldType<_Field>
                              || __is_same(_Field, bool);

template <typename _Array>
struct is_string_buffer {
    static constexpr bool value = false;
};

template <u64 _N>
struct is_string_buffer<char[_N]> {
    static constexpr bool value = true;
};

template <typename _Field>
concept CStringValueFieldType = __is_same(_Field, std::string)
                             || is_string_buffer<_Field>::value;

template <typename _Field>
concept CPrimitiveValueFieldType = CNumericValueFieldType<_Field>
                                || CStringValueFieldType<_Field>;

template <typename _Container>
concept CATRPContainerType = __is_class(_Container)
                          && requires(_Container& f_container) { f_container.upgrade().clear(); }
                          && requires(const _Container& f_container) {{ f_container.upgrade().size() } -> same_as<typename _Container::size_type>; }
                          && requires(const _Container& f_container) {{ f_container.upgrade().capacity() } -> same_as<typename _Container::size_type>; }
                          && requires(_Container& f_container, const _Container::value_type& f_value) { f_container.upgrade().push_back(f_value); }
                          && requires(_Container& f_container, _Container::value_type&& f_value) { f_container.upgrade().emplace_back(std::move(f_value)); };

template <typename _Container>
concept CNormalContainerType = __is_class(_Container)
                            && requires(_Container& f_container) { f_container.clear(); }
                            && requires(const _Container& f_container) {{ f_container.size() } -> same_as<typename _Container::size_type>; }
                            && requires(const _Container& f_container) {{ f_container.capacity() } -> same_as<typename _Container::size_type>; }
                            && requires(_Container& f_container, const _Container::value_type& f_value) { f_container.push_back(f_value); }
                            && requires(_Container& f_container, _Container::value_type&& f_value) { f_container.emplace_back(std::move(f_value)); };

template <typename _Container>
concept CContainerType = CATRPContainerType<_Container> || CNormalContainerType<_Container>;

template <typename _Container>
concept CATRPResizableContainerType = requires(_Container f_container, _Container::size_type f_new_size) { f_container.resize(f_new_size); };
template <typename _Container>
concept CNormalResizeableContainerType = requires(_Container f_container, _Container::size_type f_new_size) { f_container.upgrade().resize(f_new_size); };

template <typename _Container>
concept CResizableContainerType = CContainerType<_Container> && (CATRPResizableContainerType<_Container> || CNormalResizeableContainerType<_Container>);
} // namespace skl::config
