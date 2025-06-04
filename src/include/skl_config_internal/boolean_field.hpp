//!
//! \file boolean_field
//!
//! \license Licensed under the MIT License. See LICENSE for details.
//!
#pragma once

#include <skl_log>

#include "skl_config_internal/field.hpp"

#define SKL_LOG_TAG ""

namespace skl::config {
template <CBooleanValueFieldType _Type, CConfigTargetType _TargetConfig>
class BooleanField : public ConfigField<_TargetConfig> {
public:
    using member_ptr_t  = _Type _TargetConfig::*;
    using constraints_t = std::vector<std::function<bool(BooleanField<_Type, _TargetConfig>&, _Type)>>;

    BooleanField(Field* f_parent, std::string_view f_field_name, member_ptr_t f_member_ptr) noexcept
        : ConfigField<_TargetConfig>(f_parent, f_field_name)
        , m_member_ptr(f_member_ptr) {
    }

    ~BooleanField() override                         = default;
    BooleanField(const BooleanField&)                = default;
    BooleanField& operator=(const BooleanField&)     = default;
    BooleanField(BooleanField&&) noexcept            = default;
    BooleanField& operator=(BooleanField&&) noexcept = default;

    BooleanField& default_value(bool f_default) noexcept {
        m_default             = f_default;
        m_validate_if_default = true;
        return *this;
    }

    BooleanField& default_value(bool f_default, bool f_validate) noexcept {
        m_default             = f_default;
        m_validate_if_default = f_validate;
        return *this;
    }

    BooleanField& required(bool f_required) noexcept {
        m_required = f_required;
        return *this;
    }

    //! Default "true"
    template <u32 _N>
        requires(_N > 1U)
    BooleanField& interpret_str_true_value(const char (&f_true_str)[_N]) noexcept {
        m_true_string = f_true_str;
        return *this;
    }

    //! Default "false"
    template <u32 _N>
        requires(_N > 1U)
    BooleanField& interpret_str_false_value(const char (&f_false_str)[_N]) noexcept {
        m_false_string = f_false_str;
        return *this;
    }

    BooleanField& interpret_str(bool f_interpret_str) noexcept {
        m_interpret_str = f_interpret_str;
        return *this;
    }

    //! If the input json field is a numeric field, 0 = false , anything else = true
    BooleanField& interpret_numeric(bool f_interpret_numeric) noexcept {
        m_interpret_numeric = f_interpret_numeric;
        return *this;
    }

    template <typename _Functor>
    void add_constraint(_Functor&& f_functor) noexcept {
        m_constraints.emplace_back(std::forward<_Functor&&>(f_functor));
    }

    void reset() override {
        m_is_default         = false;
        m_is_validation_only = false;
        m_value              = std::nullopt;
    }

protected:
    void load(json& f_json) override {
        const auto exists = f_json.contains(this->name());
        if (exists) {
            auto& json = f_json[this->name()];

            if (json.is_string()) {
                if (false == m_interpret_str) {
                    SERROR_LOCAL_T("Boolean field \"{}\" cannot be interpreted from string value!", this->path_name().c_str());
                    throw std::runtime_error("Boolean field cannot be interpreted from string value!");
                }

                const auto temp = json.template get<std::string>();
                if (m_true_string == temp) {
                    m_value = true;
                } else if (m_false_string == temp) {
                    m_value = false;
                } else {
                    SERROR_LOCAL_T("Boolean field \"{}\" cannot be interpreted from the given string value(\"{}\")!", this->path_name().c_str(), skl_string_view::from_std(std::string_view{temp}));
                    throw std::runtime_error("Boolean field cannot be interpreted from the given string value!");
                }
            } else if (json.is_number()) {
                if (false == m_interpret_numeric) {
                    SERROR_LOCAL_T("Boolean field \"{}\" cannot be interpreted from numeric value!", this->path_name().c_str());
                    throw std::runtime_error("Boolean field cannot be interpreted from numeric value!");
                }

                const auto temp = json.template get<double>();
                m_value         = double(0) == temp;
            } else if (json.is_boolean()) {
                m_value = json.template get<bool>();
            } else {
                SERROR_LOCAL_T("Boolean field \"{}\"'s value cannot be interpreted as boolean!", this->path_name().c_str());
                throw std::runtime_error("Boolean field's value cannot be interpreted as boolean!");
            }

            m_is_default = false;
        } else {
            if (m_required) {
                SERROR_LOCAL_T("Boolean field \"{}\" is required!", this->path_name().c_str());
                throw std::runtime_error("Missing required field!");
            }

            if (m_default.has_value()) {
                m_value      = m_default;
                m_is_default = true;
            } else {
                SERROR_LOCAL_T("Non required boolean field \"{}\" has no default value!", this->path_name().c_str());
                throw std::runtime_error("Missing default value for required boolean field!");
            }
        }

        m_is_validation_only = false;
    }

    void validate() override {
        if (false == m_value.has_value()) {
            SKL_ASSERT((false == m_required) && (false == m_default.has_value()));
            return;
        }

        if ((false == m_is_default) || m_validate_if_default || false == m_is_validation_only) {
            //Run constraints
            for (const auto& constraint : m_constraints) {
                if (false == constraint(*this, m_value.value())) {
                    if (m_is_default) {
                        SERROR_LOCAL_T("Invalid default value({}) for boolean field\"{}\"!", m_value.value() ? "true" : "false", this->path_name().c_str());
                        throw std::runtime_error("BooleanField<T> Invalid default value");
                    } else {
                        SERROR_LOCAL_T("Invalid value({}) for boolean field\"{}\"!", m_value.value() ? "true" : "false", this->path_name().c_str());
                        throw std::runtime_error("BooleanField<T> Invalid value");
                    }
                }
            }
        }
    }

    void submit(_TargetConfig& f_config) override {
        SKL_ASSERT(m_value.has_value());
        if constexpr (__is_same(bool, _Type)) {
            f_config.*m_member_ptr = m_value.value();
        } else {
            f_config.*m_member_ptr = m_value.value() ? _Type(1) : _Type(0);
        }
    }

    void load_value_from_default_object(const _TargetConfig& f_config) override {
        if constexpr (__is_same(bool, _Type)) {
            m_value = f_config.*m_member_ptr;
        } else {
            m_value = _Type(0) != f_config.*m_member_ptr;
        }

        m_is_default         = true;
        m_is_validation_only = false;
    }

    void load_value_for_validation_only(const _TargetConfig& f_config) override {
        if constexpr (__is_same(bool, _Type)) {
            m_value = f_config.*m_member_ptr;
        } else {
            m_value = _Type(0) != f_config.*m_member_ptr;
        }

        m_is_validation_only = true;
        m_is_default         = false;
    }

    std::unique_ptr<ConfigField<_TargetConfig>> clone() override {
        return std::make_unique<BooleanField<_Type, _TargetConfig>>(*this);
    }

private:
    std::optional<bool> m_value;
    std::optional<bool> m_default;
    std::string         m_true_string  = "true";
    std::string         m_false_string = "false";
    member_ptr_t        m_member_ptr;
    constraints_t       m_constraints;
    bool                m_required{false};
    bool                m_validate_if_default{true};
    bool                m_is_default{false};
    bool                m_is_validation_only{false};
    bool                m_interpret_str{false};
    bool                m_interpret_numeric{false};

    friend ConfigNode<_TargetConfig>;
};
} // namespace skl::config

#undef SKL_LOG_TAG
