//!
//! \file object_field
//!
//! \license Licensed under the MIT License. See LICENSE for details.
//!
#pragma once

#include <skl_log>

#include "skl_config_internal/field.hpp"

#define SKL_LOG_TAG ""

namespace skl::config {
template <CConfigTargetType _Object, CConfigTargetType _TargetConfig>
class ObjectField : public ConfigField<_TargetConfig> {
public:
    using member_ptr_t  = _Object _TargetConfig::*;
    using constraints_t = std::vector<std::function<bool(ObjectField<_Object, _TargetConfig>&, const _Object&)>>;

    ObjectField(Field* f_parent, std::string_view f_field_name, member_ptr_t f_member_ptr, ConfigNode<_Object>&& f_config) noexcept
        : ConfigField<_TargetConfig>(f_parent, f_field_name)
        , m_member_ptr(f_member_ptr)
        , m_config(std::move(f_config)) {
    }

    ~ObjectField() override                        = default;
    ObjectField(const ObjectField&)                = default;
    ObjectField& operator=(const ObjectField&)     = default;
    ObjectField(ObjectField&&) noexcept            = default;
    ObjectField& operator=(ObjectField&&) noexcept = default;

    ObjectField& required(bool f_required) noexcept {
        m_required = f_required;
        return *this;
    }

    ObjectField& default_value(_Object&& f_default) {
        m_default = std::move(f_default);
        return *this;
    }

private:
    //! Load the object value from json
    void load(json& f_json) override {
        const auto exists = f_json.contains(this->name());
        if (exists) {
            if (f_json[this->name()].is_object()) {
                m_config.load(f_json[this->name()]);
                m_is_default = false;
            } else {
                SERROR_LOCAL_T("Field \"{}\" must be an object!\n\tjson: {}", this->path_name().c_str(), f_json[this->name()].dump().c_str());
                throw std::runtime_error("Wrong field type!");
            }
        } else {
            if (m_required) {
                SERROR_LOCAL_T("Object field \"{}\" is required!", this->path_name().c_str());
                throw std::runtime_error("Missing required object field!");
            }

            if (m_default.has_value()) {
                m_is_default = true;
            } else {
                SERROR_LOCAL_T("Non required object field \"{}\" has no default value!", this->path_name().c_str());
                throw std::runtime_error("Missing default value for required object field!");
            }
        }
    }

    //! Validate the field value
    void validate() override {
        if (m_is_default) {
            SKL_ASSERT(m_default.has_value());
            m_config.load_fields_from_default_object(m_default.value());
        }

        m_config.validate();
    }

    //! Submit valid value into given config object
    void submit(_TargetConfig& f_object) override {
        m_config.submit(f_object.*m_member_ptr);
    }

    void load_value_from_default_object(const _TargetConfig& f_config) override {
        m_config.load_fields_from_default_object(f_config.*m_member_ptr);
        m_is_default = true;
    }

    std::unique_ptr<ConfigField<_TargetConfig>> clone() override {
        return std::make_unique<ObjectField<_Object, _TargetConfig>>(*this);
    }

    void update_parent(Field& f_new_parent) noexcept override {
        Field::update_parent(f_new_parent);
        m_config.update_parent(f_new_parent);
    }

private:
    member_ptr_t           m_member_ptr;
    ConfigNode<_Object>    m_config;
    std::optional<_Object> m_default;
    bool                   m_required{false};
    bool                   m_validate_if_default{true};
    bool                   m_is_default{false};
};
} // namespace skl::config

#undef SKL_LOG_TAG
