//!
//! \file c_array_field
//!
//! \license Licensed under the MIT License. See LICENSE for details.
//!
#pragma once

#include <skl_log>

#include "skl_config_internal/numeric_field.hpp"
#include "skl_config_internal/string_field.hpp"

#define SKL_LOG_TAG ""

namespace skl::config {
template <CPrimitiveValueFieldType _Object, u32 _N, CConfigTargetType _TargetConfig>
class CArrayField : public ConfigField<_TargetConfig> {
public:
    struct field_value_proxy_t {
        _Object value;
    };

    template <typename _T, typename = void>
    struct field_selector_t {
        using type = NumericField<_T, field_value_proxy_t>;
    };

    template <typename _T>
        requires(CStringValueFieldType<_T>)
    struct field_selector_t<_T, void> {
        using type = StringField<_T, field_value_proxy_t, true>;
    };

    using member_ptr_t = _Object (_TargetConfig::*)[_N];
    using field_t      = field_selector_t<_Object>::type;

    CArrayField(Field* f_parent, std::string_view f_field_name, member_ptr_t f_member_ptr) noexcept
        : ConfigField<_TargetConfig>(f_parent, f_field_name)
        , m_member_ptr(f_member_ptr)
        , m_field_proto(this, "[<item>]", &field_value_proxy_t::value) {
        m_field_proto.required(true);
    }

    ~CArrayField() override                          = default;
    CArrayField(const CArrayField&)                  = default;
    CArrayField& operator=(const CArrayField&)       = default;
    CArrayField(CArrayField&&) noexcept              = default;
    CArrayField& operator=(CArrayField&&) noexcept   = default;

    CArrayField& required(bool f_required) noexcept {
        m_required = f_required;
        return *this;
    }

    CArrayField& truncate_on_overflow(bool f_truncate) noexcept {
        m_truncate_on_overflow = f_truncate;
        return *this;
    }

    [[nodiscard]] field_t& field() noexcept {
        return m_field_proto;
    }

    [[nodiscard]] std::string path_name() const noexcept override {
        if (nullptr == this->m_parent) {
            return std::string(this->name_cstr()) + "[]";
        }

        return this->m_parent->path_name() + ":" + this->name_cstr() + "[]";
    }

private:
    //! Load the field values from json
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

            // Not required and not present — leave the C-array at its default (zero-initialized) state
            m_is_default = true;
        }

        m_is_validation_only = false;
    }

    //! Validate the field values
    void validate() override {
        if (m_is_default) {
            return;
        }

        if (m_entries.size() > _N) {
            if (false == m_truncate_on_overflow) {
                SERROR_LOCAL_T("C-array field \"{}\" elements count({}) exceeds capacity({})!", this->path_name().c_str(), m_entries.size(), _N);
                throw std::runtime_error("C-array elements overflow!");
            }
        }

        const auto count = std::min(static_cast<u64>(_N), static_cast<u64>(m_entries.size()));
        for (u64 i = 0ULL; i < count; ++i) {
            m_entries[i].validate();
        }
    }

protected:
    //! Submit valid values into given config object
    void submit(_TargetConfig& f_config) override {
        auto& field = f_config.*m_member_ptr;

        // Zero-initialize the target array
        for (u32 i = 0; i < _N; ++i) {
            field[i] = _Object{};
        }

        if (m_is_default) {
            return;
        }

        const auto count = std::min(static_cast<u64>(_N), static_cast<u64>(m_entries.size()));
        for (u64 i = 0ULL; i < count; ++i) {
            field_value_proxy_t proxy{};
            m_entries[i].submit(proxy);
            field[i] = std::move(proxy.value);
        }
    }

private:
    void load_value_from_default_object(const _TargetConfig& f_config) override {
        const auto& field = f_config.*m_member_ptr;

        m_entries.clear();
        m_entries.reserve(_N);

        for (u32 i = 0; i < _N; ++i) {
            m_entries.emplace_back(m_field_proto);
            field_value_proxy_t temp{.value = field[i]};
            m_entries.back().load_value_from_default_object(temp);
        }

        m_is_default         = true;
        m_is_validation_only = false;
    }

    void load_value_for_validation_only(const _TargetConfig& f_config) override {
        const auto& field = f_config.*m_member_ptr;

        m_entries.clear();
        m_entries.reserve(_N);

        for (u32 i = 0; i < _N; ++i) {
            m_entries.emplace_back(m_field_proto);
            field_value_proxy_t temp{.value = field[i]};
            m_entries.back().load_value_for_validation_only(temp);
        }

        m_is_default         = false;
        m_is_validation_only = true;
    }

    std::unique_ptr<ConfigField<_TargetConfig>> clone() override {
        return std::make_unique<CArrayField<_Object, _N, _TargetConfig>>(*this);
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

protected:
    member_ptr_t         m_member_ptr;
    field_t              m_field_proto;
    std::vector<field_t> m_entries;
    bool                 m_required{false};
    bool                 m_is_default{false};
    bool                 m_is_validation_only{false};
    bool                 m_truncate_on_overflow{false};
};
template <CPrimitiveValueFieldType _Object, u32 _N, CConfigTargetType _TargetConfig, CIntegerValueFieldType _CountType>
class CArrayCountField : public CArrayField<_Object, _N, _TargetConfig> {
    using base_t = CArrayField<_Object, _N, _TargetConfig>;

public:
    using count_member_ptr_t = _CountType _TargetConfig::*;

    CArrayCountField(Field*                    f_parent,
                     std::string_view          f_field_name,
                     typename base_t::member_ptr_t f_member_ptr,
                     count_member_ptr_t        f_count_member_ptr) noexcept
        : base_t(f_parent, f_field_name, f_member_ptr)
        , m_count_member_ptr(f_count_member_ptr) { }

    ~CArrayCountField() override                                   = default;
    CArrayCountField(const CArrayCountField&)                      = default;
    CArrayCountField& operator=(const CArrayCountField&)           = default;
    CArrayCountField(CArrayCountField&&) noexcept                  = default;
    CArrayCountField& operator=(CArrayCountField&&) noexcept       = default;

    CArrayCountField& required(bool f_required) noexcept {
        base_t::required(f_required);
        return *this;
    }

    CArrayCountField& truncate_on_overflow(bool f_truncate) noexcept {
        base_t::truncate_on_overflow(f_truncate);
        return *this;
    }

private:
    void submit(_TargetConfig& f_config) override {
        base_t::submit(f_config);

        const auto loaded = this->m_is_default ? u64{0} : std::min(static_cast<u64>(_N), static_cast<u64>(this->m_entries.size()));
        f_config.*m_count_member_ptr = static_cast<_CountType>(loaded);
    }

    std::unique_ptr<ConfigField<_TargetConfig>> clone() override {
        return std::make_unique<CArrayCountField<_Object, _N, _TargetConfig, _CountType>>(*this);
    }

private:
    count_member_ptr_t m_count_member_ptr;
};
} // namespace skl::config

#undef SKL_LOG_TAG
