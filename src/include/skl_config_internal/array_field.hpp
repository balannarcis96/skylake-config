//!
//! \file array_field
//!
//! \license Licensed under the MIT License. See LICENSE for details.
//!
#pragma once

#include <skl_log>

#include "skl_config_internal/field.hpp"

#define SKL_LOG_TAG ""

namespace skl::config {
template <CConfigTargetType _Object, CConfigTargetType _TargetConfig, template <typename> typename _Container>
class ArrayField : public ConfigField<_TargetConfig> {
public:
    using member_ptr_t = _Container<_Object> _TargetConfig::*;

    ArrayField(Field* f_parent, std::string_view f_field_name, member_ptr_t f_member_ptr, ConfigNode<_Object>&& f_config) noexcept
        : ConfigField<_TargetConfig>(f_parent, f_field_name)
        , m_member_ptr(f_member_ptr)
        , m_config(std::move(f_config)) {
    }

    ~ArrayField() override                       = default;
    ArrayField(const ArrayField&)                = default;
    ArrayField& operator=(const ArrayField&)     = default;
    ArrayField(ArrayField&&) noexcept            = default;
    ArrayField& operator=(ArrayField&&) noexcept = default;

    ArrayField& required(bool f_required) noexcept {
        m_required = f_required;
        return *this;
    }

    ArrayField& default_value(std::vector<_Object>&& f_default) {
        m_default = std::move(f_default);
        return *this;
    }

    ArrayField& min_length(u32 f_min_length) noexcept {
        m_min_length = f_min_length;
        return *this;
    }

    ArrayField& max_length(u32 f_max_length) noexcept {
        m_max_length = f_max_length;
        return *this;
    }

    ArrayField& min_max_length(u32 f_min_length, u32 f_max_length) noexcept {
        m_min_length = f_min_length;
        m_max_length = f_max_length;
        return *this;
    }

private:
    //! Load the object value from json
    void load(json& f_json) override {
        m_entries.clear();

        const auto exists = f_json.contains(this->name());
        if (exists) {
            if (f_json[this->name()].is_array()) {
                for (auto& entry : f_json[this->name()]) {
                    m_entries.push_back(m_config);
                    m_entries.back().load(entry);
                }
                m_is_default = false;
            } else {
                SERROR_LOCAL_T("Field \"{}\" must be an array!\n\tjson: {}", this->path_name().c_str(), f_json[this->name()].dump().c_str());
                throw std::runtime_error("Wrong field type!");
            }
        } else {
            if (m_required) {
                SERROR_LOCAL_T("Array field \"{}\" is required!", this->path_name().c_str());
                throw std::runtime_error("Missing required array field!");
            }

            if (m_default.has_value()) {
                m_is_default = true;
            } else {
                SERROR_LOCAL_T("Non required array field \"{}\" has no default value!", this->path_name().c_str());
                throw std::runtime_error("Missing default value for required array field!");
            }
        }

        m_is_validation_only = false;
    }

    //! Validate the field value
    void validate() override {
        if (m_is_default) {
            SKL_ASSERT(m_default.has_value());

            m_entries.clear();

            for (const auto& field : m_default.value()) {
                m_entries.push_back(m_config);
                m_entries.back().load_fields_from_default_object(field);
            }
        }

        if ((m_entries.size() < m_min_length) || (m_entries.size() > m_max_length)) {
            SERROR_LOCAL_T("Array field \"{}\" elements count must be in [min={}, max={}]!", this->path_name().c_str(), m_min_length, m_max_length);
            throw std::runtime_error("Array field has invalid length!");
        }

        for (auto& entry : m_entries) {
            entry.validate();
        }
    }

    //! Submit valid value into given config object
    void submit(_TargetConfig& f_config) override {
        auto& field = f_config.*m_member_ptr;

        field.clear();
        field.resize(m_entries.size());

        for (u64 i = 0ULL; i < m_entries.size(); ++i) {
            m_entries[i].submit(field[i]);
        }
    }

    void load_value_from_default_object(const _TargetConfig& f_config) override {
        const auto& field = f_config.*m_member_ptr;

        m_entries.clear();
        m_entries.reserve(field.size());

        for (const auto& entry : field) {
            m_entries.push_back(m_config);
            m_entries.back().load_fields_from_default_object(entry);
        }

        m_is_default         = true;
        m_is_validation_only = false;
    }

    void load_value_for_validation_only(const _TargetConfig& f_config) override {
        const auto& field = f_config.*m_member_ptr;

        m_entries.clear();
        m_entries.reserve(field.size());

        for (const auto& entry : field) {
            m_entries.push_back(m_config);
            m_entries.back().load_fields_for_validation_only(entry);
        }

        m_is_default         = false;
        m_is_validation_only = true;
    }

    std::unique_ptr<ConfigField<_TargetConfig>> clone() override {
        return std::make_unique<ArrayField<_Object, _TargetConfig, _Container>>(*this);
    }

    void update_parent(Field& f_new_parent) noexcept override {
        Field::update_parent(f_new_parent);
        m_config.update_parent(f_new_parent);

        for (auto& entry : m_entries) {
            entry.update_parent(f_new_parent);
        }
    }

    void reset() override {
        m_entries.clear();
        m_is_default         = false;
        m_is_validation_only = false;
    }

private:
    member_ptr_t                        m_member_ptr;
    ConfigNode<_Object>                 m_config;
    std::vector<ConfigNode<_Object>>    m_entries;
    std::optional<std::vector<_Object>> m_default;
    u32                                 m_min_length = 0U;
    u32                                 m_max_length = std::numeric_limits<u32>::max();
    bool                                m_required{false};
    bool                                m_is_default{false};
    bool                                m_is_validation_only{false};
};
} // namespace skl::config

#undef SKL_LOG_TAG
