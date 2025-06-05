//!
//! \file numeric_field
//!
//! \license Licensed under the MIT License. See LICENSE for details.
//!
#pragma once

#include <charconv>

#include <skl_log>

#include "skl_config_internal/field.hpp"

#define SKL_LOG_TAG ""

namespace skl::config {
template <CNumericValueFieldType _Type, CConfigTargetType _TargetConfig>
class NumericField : public ConfigField<_TargetConfig> {
public:
    using member_ptr_t   = _Type _TargetConfig::*;
    using constraints_t  = std::vector<std::function<bool(NumericField<_Type, _TargetConfig>&, _Type)>>;
    using raw_parsert_t  = std::function<std::optional<_Type>(NumericField<_Type, _TargetConfig>&, std::string_view)>;
    using json_parsert_t = std::function<std::optional<_Type>(NumericField<_Type, _TargetConfig>&, json&)>;

    NumericField(Field* f_parent, std::string_view f_field_name, member_ptr_t f_member_ptr) noexcept
        : ConfigField<_TargetConfig>(f_parent, f_field_name)
        , m_member_ptr(f_member_ptr) {
    }

    ~NumericField() override                         = default;
    NumericField(const NumericField&)                = default;
    NumericField& operator=(const NumericField&)     = default;
    NumericField(NumericField&&) noexcept            = default;
    NumericField& operator=(NumericField&&) noexcept = default;

    NumericField& default_value(_Type f_default) noexcept {
        m_default             = f_default;
        m_validate_if_default = true;
        return *this;
    }

    NumericField& default_value(_Type f_default, bool f_validate) noexcept {
        m_default             = f_default;
        m_validate_if_default = f_validate;
        return *this;
    }

    NumericField& required(bool f_required) noexcept {
        m_required = f_required;
        return *this;
    }

    NumericField& min(_Type f_min) noexcept {
        add_constraint([f_min](auto& f_self, _Type f_value) {
            if (f_value < f_min) {
                SERROR("Invalid numeric field \"{}\" value! Min[{}]!", f_self.name_cstr(), f_min);
                return false;
            }
            return true;
        });
        return *this;
    }

    NumericField& max(_Type f_max) noexcept {
        add_constraint([f_max](auto& f_self, _Type f_value) {
            if (f_value > f_max) {
                SERROR("Invalid numeric field \"{}\" value! Max[{}]!", f_self.name_cstr(), f_max);
                return false;
            }

            return true;
        });
        return *this;
    }

    NumericField& power_of_2() noexcept
        requires(CIntegerValueFieldType<_Type>)
    {
        add_constraint([](auto& f_self, _Type f_value) static {
            if ((f_value < 2U) || (_Type(0) != ((f_value - _Type(1)) & f_value))) {
                SERROR("Invalid numeric field \"{}\" value({}) must be a power of 2! Min[2]!", f_self.name_cstr(), f_value);
                return false;
            }
            return true;
        });
        return *this;
    }

    //! Set custom raw json value string parser
    //! \remark (auto& f_self, std::string_view f_string) -> std::optional<_Type>
    template <typename _Functor>
    NumericField& parse_raw(_Functor&& f_functor) noexcept {
        m_custom_raw_parser = std::forward<_Functor>(f_functor);
        return *this;
    }

    //! Set custom json node parser
    //! \remark (auto& f_self, json& f_json) -> std::optional<_Type>
    template <typename _Functor>
    NumericField& parse_json(_Functor&& f_functor) noexcept {
        m_custom_json_parser = std::forward<_Functor>(f_functor);
        return *this;
    }

    //! Add custom constraint
    //! \remark (auto& f_self, _Type f_value) -> bool
    template <typename _Functor>
    void add_constraint(_Functor&& f_functor) noexcept {
        m_constraints.emplace_back(std::forward<_Functor&&>(f_functor));
    }

    void reset() override {
        m_is_default         = false;
        m_is_validation_only = false;
        m_value              = std::nullopt;
    }

    [[nodiscard]] static std::optional<_Type> safely_convert_to_numeric(std::string_view f_str)
        requires(CNumericValueFieldType<_Type>)
    {
        _Type value;
        auto [ptr, ec] = std::from_chars(f_str.data(), f_str.data() + f_str.size(), value);
        if (ec != std::errc{}) {
            return std::nullopt;
        }

        return value;
    }

protected:
    void load(json& f_json) override {
        const auto exists = f_json.contains(this->name());
        if (exists) {
            if (false == m_custom_json_parser.has_value()) {
                const auto temp = f_json[this->name()].is_string() ? f_json[this->name()].template get<std::string>() : f_json[this->name()].dump();
                if (m_custom_raw_parser.has_value()) {
                    const auto result = m_custom_raw_parser.value()(*this, temp);
                    if (false == result.has_value()) {
                        throw std::runtime_error("Custom parsing for numeric field failed!");
                    }

                    m_value = result;
                } else {
                    const auto result = safely_convert_to_numeric(temp);
                    if (false == result.has_value()) {
                        SERROR_LOCAL_T("Numeric field \"{}\" has an invalid {} value({})! Min[{}] Max[{}]",
                                       this->path_name().c_str(),
                                       (false == __is_same(_Type, float)) ? (__is_same(_Type, double) ? "double" : "integer") : "float",
                                       f_json[this->name()].dump().c_str(),
                                       std::numeric_limits<_Type>::min(),
                                       std::numeric_limits<_Type>::max());
                        throw std::runtime_error("Invalid numeric field value!");
                    } else {
                        m_value = result.value();
                    }
                }
            } else {
                const auto result = m_custom_json_parser.value()(*this, f_json[this->name()]);
                if (false == result.has_value()) {
                    throw std::runtime_error("Custom json parsing for numeric field failed!");
                }

                m_value = result;
            }

            m_is_default = false;
        } else {
            if (m_required) {
                SERROR_LOCAL_T("Numeric field \"{}\" is required!", this->path_name().c_str());
                throw std::runtime_error("Missing required field!");
            }

            if (m_default.has_value()) {
                m_value      = m_default.value();
                m_is_default = true;
            } else {
                SERROR_LOCAL_T("Non required numeric field \"{}\" has no default value!", this->path_name().c_str());
                throw std::runtime_error("Missing default value for required numeric field!");
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
                        SERROR_LOCAL_T("Invalid default value({}) for numeric field\"{}\"!", m_value.value(), this->path_name().c_str());
                        throw std::runtime_error("NumericField<T> Invalid default value");
                    } else {
                        SERROR_LOCAL_T("Invalid value({}) for numeric field\"{}\"!", m_value.value(), this->path_name().c_str());
                        throw std::runtime_error("NumericField<T> Invalid value");
                    }
                }
            }
        }
    }

    void submit(_TargetConfig& f_config) override {
        SKL_ASSERT(m_value.has_value());
        f_config.*m_member_ptr = m_value.value();
    }

    void load_value_from_default_object(const _TargetConfig& f_config) override {
        m_value              = f_config.*m_member_ptr;
        m_is_default         = true;
        m_is_validation_only = false;
    }

    void load_value_for_validation_only(const _TargetConfig& f_config) override {
        m_value              = f_config.*m_member_ptr;
        m_is_validation_only = true;
        m_is_default         = false;
    }

    std::unique_ptr<ConfigField<_TargetConfig>> clone() override {
        return std::make_unique<NumericField<_Type, _TargetConfig>>(*this);
    }

private:
    std::optional<_Type>          m_value;
    std::optional<_Type>          m_default;
    std::optional<raw_parsert_t>  m_custom_raw_parser;
    std::optional<json_parsert_t> m_custom_json_parser;
    member_ptr_t                  m_member_ptr;
    constraints_t                 m_constraints;
    bool                          m_required{false};
    bool                          m_validate_if_default{true};
    bool                          m_is_default{false};
    bool                          m_is_validation_only{false};

    friend ConfigNode<_TargetConfig>;
};
} // namespace skl::config

#undef SKL_LOG_TAG
