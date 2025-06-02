//!
//! \file array_field
//!
//! \license Licensed under the MIT License. See LICENSE for details.
//!
#pragma once

#include "skl_config_internal/field.hpp"

#define SKL_LOG_TAG "[ArrayField<T>] -- "

namespace skl::config {
template <CValueFieldType _Type, CConfigTargetType _TargetConfig>
class ArrayField : public ConfigField<_TargetConfig> {
public:
    using member_ptr_t  = _Type _TargetConfig::*;
    using constraints_t = std::vector<std::function<bool(ArrayField<_Type, _TargetConfig>&, const _Type&)>>;

    ArrayField(Field* f_parent, std::string_view f_field_name, member_ptr_t f_member_ptr) noexcept
        : ConfigField<_TargetConfig>(f_parent, f_field_name)
        , m_member_ptr(f_member_ptr) {
        add_default_constraints();
    }

    ~ArrayField() override                       = default;
    ArrayField(const ArrayField&)                = default;
    ArrayField& operator=(const ArrayField&)     = default;
    ArrayField(ArrayField&&) noexcept            = default;
    ArrayField& operator=(ArrayField&&) noexcept = default;

private:
    void add_default_constraints() {
    }

private:
    std::optional<_Type> m_value;
    std::optional<_Type> m_default;
    member_ptr_t         m_member_ptr;
    constraints_t        m_constraints;
    bool                 m_required{false};
    bool                 m_validate_if_default{true};
    bool                 m_is_default{false};
};
} // namespace skl::config

#undef SKL_LOG_TAG
