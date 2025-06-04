//!
//! \file field
//!
//! \license Licensed under the MIT License. See LICENSE for details.
//!
#pragma once

#include <nlohmann/json.hpp>

#include "skl_config_internal/common.hpp"

namespace skl {
template <config::CConfigTargetType _TargetConfig>
class ConfigNode;
}

namespace skl::config {
using json = nlohmann::json;

class Field {
public:
    Field(Field* f_parent, std::string_view f_name) noexcept
        : m_name(f_name)
        , m_parent(f_parent) { }

    virtual ~Field()                   = default;
    Field(const Field&)                = default;
    Field& operator=(const Field&)     = default;
    Field(Field&&) noexcept            = default;
    Field& operator=(Field&&) noexcept = default;

    [[nodiscard]] std::string_view name() const noexcept {
        return m_name;
    }

    [[nodiscard]] const char* name_cstr() const noexcept {
        return m_name.c_str();
    }

    [[nodiscard]] Field* parent_field() noexcept {
        return m_parent;
    }

    [[nodiscard]] const Field* parent_field() const noexcept {
        return m_parent;
    }

    [[nodiscard]] std::string path_name() const noexcept {
        if (nullptr == m_parent) {
            return name_cstr();
        }

        return m_parent->path_name() + ":" + name_cstr();
    }

    virtual void reset() = 0;

protected:
    virtual void update_parent(Field& f_new_parent) noexcept {
        m_parent = &f_new_parent;
    }

protected:
    std::string m_name;
    Field*      m_parent;
};

template <CConfigTargetType _TargetConfig>
class ConfigField : public Field {
public:
    ConfigField(Field* f_parent, std::string_view f_name) noexcept
        : Field(f_parent, f_name) { }

    ~ConfigField() override                        = default;
    ConfigField(const ConfigField&)                = default;
    ConfigField& operator=(const ConfigField&)     = default;
    ConfigField(ConfigField&&) noexcept            = default;
    ConfigField& operator=(ConfigField&&) noexcept = default;

protected:
    //! Load the field value from json
    virtual void load(json&) = 0;

    //! Validate the field value
    virtual void validate() = 0;

    //! Submit valid value into given config object
    virtual void submit(_TargetConfig&) = 0;

    //! Load value from default object
    virtual void load_value_from_default_object(const _TargetConfig&) = 0;

    //! Load value for validation only
    virtual void load_value_for_validation_only(const _TargetConfig&) = 0;

    //! Clone this field
    virtual std::unique_ptr<ConfigField<_TargetConfig>> clone() = 0;

    friend ConfigNode<_TargetConfig>;
};
} // namespace skl::config
