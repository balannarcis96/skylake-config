//!
//! \file enumc_field
//!
//! \license Licensed under the MIT License. See LICENSE for details.
//!
#pragma once

#include <skl_log>
#include <skl_magic_enum>

#include "skl_config_internal/field.hpp"

#define SKL_LOG_TAG ""

namespace skl::config {
template <CEnumValueFieldType _Type, CConfigTargetType _TargetConfig>
class EnumField;

template <typename _Type, typename _Functor>
concept CEnumFieldParseRawFunctor = __is_class(_Functor)
                                 && std::is_invocable_r_v<std::optional<_Type>, _Functor, Field&, const std::string&>;

template <typename _Type, typename _Functor>
concept CEnumFieldConstraintFunctor = __is_class(_Functor)
                                   && std::is_invocable_r_v<bool, _Functor, Field&, _Type>;

template <typename _Type, typename _Functor>
concept CEnumFieldParseJsonFunctor = __is_class(_Functor)
                                  && std::is_invocable_r_v<std::optional<_Type>, _Functor, Field&, json&>;

template <typename _Type, typename _Functor>
concept CEnumFieldPostLoadFunctor = __is_class(_Functor)
                                 && std::is_invocable_r_v<bool, _Functor, Field&, _Type>;

template <typename _Type, typename _Functor, typename _TargetConfig>
concept CEnumFieldPreSubmitFunctor = __is_class(_Functor)
                                  && std::is_invocable_r_v<bool, _Functor, Field&, _Type, _TargetConfig&>;

template <CEnumValueFieldType _Type, CConfigTargetType _TargetConfig>
class EnumField : public ConfigField<_TargetConfig> {
public:
    using member_ptr_t   = _Type _TargetConfig::*;
    using constraints_t  = std::vector<std::function<bool(Field&, _Type)>>;
    using raw_parsert_t  = std::function<std::optional<_Type>(Field&, const std::string&)>;
    using json_parsert_t = std::function<std::optional<_Type>(Field&, json&)>;
    using post_load_t    = std::function<bool(Field&, _Type)>;
    using pre_submit_t   = std::function<bool(Field&, _Type, _TargetConfig&)>;
    using underlying_t   = __underlying_type(_Type);

    EnumField(Field* f_parent, std::string_view f_field_name, member_ptr_t f_member_ptr) noexcept
        : ConfigField<_TargetConfig>(f_parent, f_field_name)
        , m_member_ptr(f_member_ptr) {
    }

    ~EnumField() override                      = default;
    EnumField(const EnumField&)                = default;
    EnumField& operator=(const EnumField&)     = default;
    EnumField(EnumField&&) noexcept            = default;
    EnumField& operator=(EnumField&&) noexcept = default;

    EnumField& default_value(_Type f_default) noexcept {
        m_default             = f_default;
        m_validate_if_default = true;
        return *this;
    }

    EnumField& default_value(_Type f_default, bool f_validate) noexcept {
        m_default             = f_default;
        m_validate_if_default = f_validate;
        return *this;
    }

    EnumField& required(bool f_required) noexcept {
        m_required = f_required;
        return *this;
    }

    //! Set custom raw json value string parser
    //! \remark (Field& f_self, std::string_view f_string) -> std::optional<_Type>
    template <typename _Functor>
        requires(CEnumFieldParseRawFunctor<_Type, _Functor>)
    EnumField& parse_raw(_Functor&& f_functor) noexcept {
        m_custom_raw_parser = std::forward<_Functor>(f_functor);
        return *this;
    }

    //! Set custom raw json value string parser
    //! \remark (Field& f_self, std::string_view f_string) static -> std::optional<_Type>
    template <typename _Functor>
        requires(CEnumFieldParseRawFunctor<_Type, _Functor>)
    EnumField& parse_raw() noexcept {
        m_custom_raw_parser = raw_parsert_t(&_Functor::operator());
        return *this;
    }

    //! Set custom json node parser
    //! \remark (Field& f_self, json& f_json) -> std::optional<_Type>
    template <typename _Functor>
        requires(CEnumFieldParseJsonFunctor<_Type, _Functor>)
    EnumField& parse_json(_Functor&& f_functor) noexcept {
        m_custom_json_parser = std::forward<_Functor>(f_functor);
        return *this;
    }

    //! Set custom json node parser
    //! \remark (Field& f_self, json& f_json) static -> std::optional<_Type>
    template <typename _Functor>
        requires(CEnumFieldParseJsonFunctor<_Type, _Functor>)
    EnumField& parse_json() noexcept {
        m_custom_json_parser = &_Functor::operator();
        return *this;
    }

    //! Set post load handler
    //! \remark (Field& f_self, _Type f_val) -> bool
    template <typename _Functor>
        requires(CEnumFieldPostLoadFunctor<_Type, _Functor>)
    EnumField& post_load(_Functor&& f_functor) noexcept {
        m_post_load = std::forward<_Functor>(f_functor);
        return *this;
    }

    //! Set post load handler
    //! \remark (Field& f_self, _Type f_val) static -> bool
    template <typename _Functor>
        requires(CEnumFieldPostLoadFunctor<_Type, _Functor>)
    EnumField& post_load() noexcept {
        m_post_load = &_Functor::operator();
        return *this;
    }

    //! Set pre submit handler
    //! \remark (Field& f_self, _Type f_val, _TargetConfig& f_config) -> bool
    template <typename _Functor>
        requires(CEnumFieldPreSubmitFunctor<_Type, _Functor, _TargetConfig>)
    EnumField& pre_submit(_Functor&& f_functor) noexcept {
        m_pre_submit = std::forward<_Functor>(f_functor);
        return *this;
    }

    //! Set pre submit handler
    //! \remark (Field& f_self, const std::string& f_value) static -> bool
    template <typename _Functor>
        requires(CEnumFieldPreSubmitFunctor<_Type, _Functor, _TargetConfig>)
    EnumField& post_load() noexcept {
        m_pre_submit = &_Functor::operator();
        return *this;
    }

    //! Add custom constraint
    //! \remark (Field& f_self, _Type f_value) static -> bool
    template <typename _Functor>
        requires(CEnumFieldConstraintFunctor<_Type, _Functor>)
    void add_constraint() noexcept {
        m_constraints.emplace_back(&_Functor::operator());
    }

    //! Add custom constraint
    //! \remark (Field& f_self, _Type f_value) -> bool
    template <typename _Functor>
        requires(CEnumFieldConstraintFunctor<_Type, _Functor>)
    void add_constraint(const _Functor& f_functor) noexcept {
        m_constraints.emplace_back(f_functor);
    }

    //! Add custom constraint
    //! \remark (Field& f_self, _Type f_value) -> bool
    template <typename _Functor>
        requires(CEnumFieldConstraintFunctor<_Type, _Functor>)
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
        bool  exists   = false;
        json* src_json = nullptr;

        if (f_json.is_string()) {
            exists   = true;
            src_json = &f_json;
        } else {
            exists   = f_json.contains(this->name());
            src_json = &f_json[this->name()];
        }

        if (exists) {
            if (false == src_json->is_string()) {
                SERROR_LOCAL_T("Enum field \"{}\" must have a string value!", this->path_name().c_str());
                throw std::runtime_error("Enum field not a string!");
            }

            if (false == m_custom_json_parser.has_value()) {
                if (m_custom_raw_parser.has_value()) {
                    const auto result = m_custom_raw_parser.value()(*this, src_json->template get<std::string>());
                    if (false == result.has_value()) {
                        throw std::runtime_error("Custom parsing for enum field failed!");
                    }

                    m_value = result;
                } else {
                    const auto result = enum_from_string<_Type>(src_json->template get<std::string>());
                    if (false == result.has_value()) {
                        SERROR_LOCAL_T("Enum field \"{}\" has invalid value({})!",
                                       this->path_name().c_str(),
                                       src_json->dump().c_str());
                        puts("\tAllowed values:");
                        for (_Type value : magic_enum::enum_values<_Type>()) {
                            printf("\t\t%s\n", magic_enum::enum_name(value).data());
                        }

                        throw std::runtime_error("Invalid enum field value!");
                    }

                    m_value = result.value();
                }
            } else {
                const auto result = m_custom_json_parser.value()(*this, *src_json);
                if (false == result.has_value()) {
                    throw std::runtime_error("Custom json parsing for enum field failed!");
                }

                m_value = result;
            }

            if (m_post_load.has_value()) {
                if (false == m_post_load.value()(*this, m_value.value())) {
                    SERROR_LOCAL_T("Field \"{}\" failed post load!", this->path_name().c_str());
                    throw std::runtime_error("Enum field failed post load!");
                }
            }

            m_is_default = false;
        } else {
            if (m_required) {
                SERROR_LOCAL_T("Enum field \"{}\" is required!", this->path_name().c_str());
                throw std::runtime_error("Missing required enum field!");
            }

            if (m_default.has_value()) {
                m_value      = m_default.value();
                m_is_default = true;
            } else {
                SERROR_LOCAL_T("Non required enum field \"{}\" has no default value!", this->path_name().c_str());
                throw std::runtime_error("Missing default value for required enum field!");
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
            if (false == magic_enum::enum_contains<_Type>(m_value.value())) {
                SERROR_LOCAL_T("Invalid value({}) for enum field\"{}\"!", underlying_t(m_value.value()), this->path_name().c_str());
                throw std::runtime_error("EnumField<T> Invalid default value");
            }

            //Run constraints
            for (const auto& constraint : m_constraints) {
                if (false == constraint(*this, m_value.value())) {
                    if (m_is_default) {
                        SERROR_LOCAL_T("[Constraint] Invalid default value({}) for enum field\"{}\"!", underlying_t(m_value.value()), this->path_name().c_str());
                        throw std::runtime_error("EnumField<T> Invalid default value");
                    } else {
                        SERROR_LOCAL_T("[Constraint] Invalid value({}) for enum field\"{}\"!", underlying_t(m_value.value()), this->path_name().c_str());
                        throw std::runtime_error("[Constraint] EnumField<T> Invalid value");
                    }
                }
            }
        }
    }

    void submit(_TargetConfig& f_config) override {
        SKL_ASSERT(m_value.has_value());

        if (m_pre_submit.has_value()) {
            if (false == m_pre_submit.value()(*this, m_value.value(), f_config)) {
                SERROR_LOCAL_T("NumericFiled \"{}\" pre_submit handler failed!", this->path_name().c_str());
                throw std::runtime_error("NumericFiled pre_submit handler failed!");
            }
        }

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
        return std::make_unique<EnumField<_Type, _TargetConfig>>(*this);
    }

private:
    std::optional<_Type>          m_value;
    std::optional<_Type>          m_default;
    std::optional<raw_parsert_t>  m_custom_raw_parser;
    std::optional<json_parsert_t> m_custom_json_parser;
    std::optional<post_load_t>    m_post_load;
    std::optional<pre_submit_t>   m_pre_submit;
    member_ptr_t                  m_member_ptr;
    constraints_t                 m_constraints;
    bool                          m_required{false};
    bool                          m_validate_if_default{true};
    bool                          m_is_default{false};
    bool                          m_is_validation_only{false};

    friend ConfigNode<_TargetConfig>;

    template <CPrimitiveValueFieldType, CConfigTargetType, config::CContainerType>
    friend class PrimitiveArrayField;
};
} // namespace skl::config

#undef SKL_LOG_TAG
