//!
//! \file primitive_array_field
//!
//! \license Licensed under the MIT License. See LICENSE for details.
//!
#pragma once

#include <skl_log>
#include <skl_traits/conditional_t>

#include "skl_config_internal/numeric_field.hpp"
#include "skl_config_internal/string_field.hpp"

#define SKL_LOG_TAG ""

namespace skl::config {
template <CPrimitiveValueFieldType _Object, CConfigTargetType _TargetConfig, template <typename> typename _Container>
class PrimitiveArrayField : public ConfigField<_TargetConfig> {
public:
    struct field_value_proxy_t {
        _Object value;
    };

    template <typename _T, typename = void>
    struct field_selector_t {
        using type = NumericField<_T, field_value_proxy_t>;
    };

    template <typename _T>
    struct field_selector_t<StringField<_T, field_value_proxy_t>> {
        using type = StringField<_T, field_value_proxy_t>;
    };

    using member_ptr_t = _Container<_Object> _TargetConfig::*;
    using field_t      = field_selector_t<_Object>::type;

    PrimitiveArrayField(Field* f_parent, std::string_view f_field_name, member_ptr_t f_member_ptr) noexcept
        : ConfigField<_TargetConfig>(f_parent, f_field_name)
        , m_member_ptr(f_member_ptr)
        , m_field_proto(this, "[<item>]", &field_value_proxy_t::value) {
        m_field_proto.required(true);
    }

    ~PrimitiveArrayField() override                                = default;
    PrimitiveArrayField(const PrimitiveArrayField&)                = default;
    PrimitiveArrayField& operator=(const PrimitiveArrayField&)     = default;
    PrimitiveArrayField(PrimitiveArrayField&&) noexcept            = default;
    PrimitiveArrayField& operator=(PrimitiveArrayField&&) noexcept = default;

    PrimitiveArrayField& required(bool f_required) noexcept {
        m_required = f_required;
        return *this;
    }

    PrimitiveArrayField& default_value(const std::vector<_Object>& f_default) {
        m_default = std::vector<field_value_proxy_t>();
        m_default->resize(f_default.size());

        for (u64 i = 0ULL; i < f_default.size(); ++i) {
            m_default.value()[i].value = f_default[i];
        }

        return *this;
    }

    PrimitiveArrayField& min_length(u32 f_min_length) noexcept {
        m_min_length = f_min_length;
        return *this;
    }

    PrimitiveArrayField& max_length(u32 f_max_length) noexcept {
        m_max_length = f_max_length;
        return *this;
    }

    PrimitiveArrayField& min_max_length(u32 f_min_length, u32 f_max_length) noexcept {
        m_min_length = f_min_length;
        m_max_length = f_max_length;
        return *this;
    }

    [[nodiscard]] field_t& field() noexcept {
        return m_field_proto;
    }

private:
    //! Load the object value from json
    void load(json& f_json) override {
        m_entries.clear();

        const auto exists = f_json.contains(this->name());
        if (exists) {
            if (f_json[this->name()].is_array()) {
                for (auto& entry : f_json[this->name()]) {
                    m_entries.push_back(m_field_proto);
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
                m_entries.push_back(m_field_proto);
                m_entries.back().load_value_from_default_object(field);
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
            field_value_proxy_t proxy{};
            m_entries[i].submit(proxy);
            field[i] = proxy.value;
        }
    }

    void load_value_from_default_object(const _TargetConfig& f_config) override {
        const auto& field = f_config.*m_member_ptr;

        m_entries.clear();
        m_entries.reserve(field.size());

        for (const auto& entry : field) {
            m_entries.emplace_back(m_field_proto);
            field_value_proxy_t temp{.value = entry};
            m_entries.back().load_value_from_default_object(temp);
        }

        m_is_default         = true;
        m_is_validation_only = false;
    }

    void load_value_for_validation_only(const _TargetConfig& f_config) override {
        const auto& field = f_config.*m_member_ptr;

        m_entries.clear();
        m_entries.reserve(field.size());

        for (const auto& entry : field) {
            m_entries.emplace_back(m_field_proto);
            field_value_proxy_t temp{.value = entry};
            m_entries.back().load_value_for_validation_only(temp);
        }

        m_is_default         = false;
        m_is_validation_only = true;
    }

    std::unique_ptr<ConfigField<_TargetConfig>> clone() override {
        return std::make_unique<PrimitiveArrayField<_Object, _TargetConfig, _Container>>(*this);
    }

    void update_parent(Field& f_new_parent) noexcept override {
        Field::update_parent(f_new_parent);
        m_field_proto.update_parent(f_new_parent);

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
    member_ptr_t                                    m_member_ptr;
    field_t                                         m_field_proto;
    std::vector<field_t>                            m_entries;
    std::optional<std::vector<field_value_proxy_t>> m_default;
    u32                                             m_min_length = 0U;
    u32                                             m_max_length = std::numeric_limits<u32>::max();
    bool                                            m_required{false};
    bool                                            m_is_default{false};
    bool                                            m_is_validation_only{false};
};
} // namespace skl::config

#undef SKL_LOG_TAG
