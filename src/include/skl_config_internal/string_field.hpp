//!
//! \file string_field
//!
//! \license Licensed under the MIT License. See LICENSE for details.
//!
#pragma once

#include <skl_log>

#include "skl_config_internal/field.hpp"

#define SKL_LOG_TAG ""

namespace skl::config {
template <typename _Functor>
concept CStringFieldConstraintFunctor = __is_class(_Functor)
                                     && std::is_invocable_r_v<bool, _Functor, Field&, const std::string&>;
template <typename _Functor>
concept CStringFieldPostLoadFunctor = __is_class(_Functor)
                                   && std::is_invocable_r_v<bool, _Functor, Field&, const std::string&>;
template <CStringValueFieldType _Type, CConfigTargetType _TargetConfig>
class StringField : public ConfigField<_TargetConfig> {
public:
    using member_ptr_t  = _Type _TargetConfig::*;
    using constraints_t = std::vector<std::function<bool(Field&, const std::string&)>>;
    using post_load_t   = std::function<bool(Field&, const std::string&)>;

    StringField(Field* f_parent, std::string_view f_field_name, member_ptr_t f_member_ptr) noexcept
        requires(__is_same(std::string, _Type))
        : ConfigField<_TargetConfig>(f_parent, f_field_name)
        , m_member_ptr(f_member_ptr) {
    }

    template <typename _TChar, u32 _N>
    StringField(Field* f_parent, std::string_view f_field_name, _TChar (_TargetConfig::*f_member_ptr)[_N]) noexcept
        requires((__is_same(_TChar, char) || __is_same(_TChar, unsigned char)) && (_N > 1U))
        : ConfigField<_TargetConfig>(f_parent, f_field_name)
        , m_member_ptr(f_member_ptr)
        , m_buffer_size(_N) {
    }

    ~StringField() override                        = default;
    StringField(const StringField&)                = default;
    StringField& operator=(const StringField&)     = default;
    StringField(StringField&&) noexcept            = default;
    StringField& operator=(StringField&&) noexcept = default;

    template <u32 _N>
        requires(_N > 0U)
    StringField& default_value(const char (&f_default_string)[_N]) {
        return default_value(skl_string_view::exact_cstr(f_default_string).template std<std::string_view>());
    }

    StringField& default_value(std::string_view f_default_string) {
        if constexpr (false == __is_same(std::string, _Type)) {
            if (f_default_string.length() > (m_buffer_size - 1U)) {
                SERROR_LOCAL_T("[Setup] StringField<char[{}]> default value(\"{}\" length={}) doesn't fit inside the buffer!", m_buffer_size, skl_string_view::from_std(f_default_string), f_default_string.length());
                throw std::runtime_error("[Setup] StringField<char[N]> default value doesn't fit inside the buffer!");
            }
        }

        m_default             = f_default_string;
        m_validate_if_default = true;
        return *this;
    }

    StringField& default_value(std::string_view f_default_string, bool f_validate) {
        if constexpr (false == __is_same(std::string, _Type)) {
            if (f_default_string.length() > (m_buffer_size - 1U)) {
                SERROR_LOCAL_T("[Setup] StringField<char[{}]> default value(\"{}\" length={}) doesn't fit inside the buffer!", m_buffer_size, skl_string_view::from_std(f_default_string), f_default_string.length());
                throw std::runtime_error("[Setup] StringField<char[N]> default value doesn't fit inside the buffer!");
            }
        }

        m_default             = f_default_string;
        m_validate_if_default = f_validate;
        return *this;
    }

    StringField& default_value(std::string&& default_val) {
        if constexpr (false == __is_same(std::string, _Type)) {
            if (default_val.length() > (m_buffer_size - 1U)) {
                SERROR_LOCAL_T("[Setup] StringField<char[{}]> default value(\"{}\" length={}) doesn't fit inside the buffer!", m_buffer_size, default_val.c_str(), default_val.length());
                throw std::runtime_error("[Setup] StringField<char[N]> default value doesn't fit inside the buffer!");
            }
        }

        m_default             = std::move(default_val);
        m_validate_if_default = true;
        return *this;
    }

    StringField& default_value(std::string&& default_val, bool f_validate) {
        if constexpr (false == __is_same(std::string, _Type)) {
            if (default_val.length() > (m_buffer_size - 1U)) {
                SERROR_LOCAL_T("[Setup] StringField<char[{}]> default value(\"{}\" length={}) doesn't fit inside the buffer!", m_buffer_size, default_val.c_str(), default_val.length());
                throw std::runtime_error("[Setup] StringField<char[N]> default value doesn't fit inside the buffer!");
            }
        }

        m_default             = std::move(default_val);
        m_validate_if_default = f_validate;
        return *this;
    }

    StringField& required(bool f_required) noexcept {
        m_required = f_required;
        return *this;
    }

    StringField& truncate_to_buffer(bool f_truncate_to_buffer) noexcept
        requires(false == __is_same(std::string, _Type))
    {
        m_truncate_to_buffer = f_truncate_to_buffer;
        return *this;
    }

    //! Dump the json as string if the json field is not of type string
    StringField& dump_if_not_string(bool f_dump_if_not_string) noexcept {
        m_dump_if_not_string = f_dump_if_not_string;
        return *this;
    }

    StringField& min_length(u32 f_min_length) {
        if constexpr (false == __is_same(std::string, _Type)) {
            if (f_min_length > (m_buffer_size - 1U)) {
                SERROR_LOCAL_T("[Setup] StringField<char[{}]> min length constraint value({}) outside of buffer length!", m_buffer_size, f_min_length);
                throw std::runtime_error("[Setup] StringField<char[N]> min length constraint value() outside of buffer length!");
            }
        }

        add_constraint([f_min_length](auto& f_self, const std::string& f_value) {
            if (f_value.length() < f_min_length) {
                SERROR("Invalid string field \"{}\" value length! Min[{}]!", f_self.name_cstr(), f_min_length);
                return false;
            }
            return true;
        });
        return *this;
    }

    StringField& max_length(u32 f_max_length) {
        if constexpr (false == __is_same(std::string, _Type)) {
            if (f_max_length > (m_buffer_size - 1U)) {
                SERROR_LOCAL_T("[Setup] StringField<char[{}]> max length constraint value({}) outside of buffer length!", m_buffer_size, f_max_length);
                throw std::runtime_error("[Setup] StringField<char[N]> max length constraint value() outside of buffer length!");
            }
        }

        add_constraint([f_max_length](auto& f_self, const std::string& f_value) {
            if (f_value.length() > f_max_length) {
                SERROR("Invalid string field \"{}\" value length! Max[{}]!", f_self.name_cstr(), f_max_length);
                return false;
            }
            return true;
        });
        return *this;
    }

    //! Add post load handler
    //! \remark (Field& f_self, const std::string& f_value) -> bool
    template <CStringFieldPostLoadFunctor _Functor>
    StringField& post_load(_Functor&& f_functor) noexcept {
        m_post_load = std::forward<_Functor>(f_functor);
        return *this;
    }

    //! Add post load handler
    //! \remark (Field& f_self, const std::string& f_value) static -> bool
    template <CStringFieldPostLoadFunctor _Functor>
    StringField& post_load() noexcept {
        m_post_load = &_Functor::operator();
        return *this;
    }

    //! Add custom constraint
    //! \remark (Field& f_self, const std::string& f_value) -> bool
    template <CStringFieldConstraintFunctor _Functor>
    void add_constraint(_Functor&& f_functor) noexcept {
        m_constraints.emplace_back(std::forward<_Functor&&>(f_functor));
    }

    //! Add custom constraint
    //! \remark (Field& f_self, const std::string& f_value) -> bool
    template <CStringFieldConstraintFunctor _Functor>
    void add_constraint() noexcept {
        m_constraints.emplace_back(&_Functor::operator());
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
                m_value = json.template get<std::string>();
            } else {
                if (m_dump_if_not_string) {
                    m_value = json.dump();
                } else {
                    SERROR_LOCAL_T("Field \"{}\" must be a string field!", this->path_name().c_str());
                    throw std::runtime_error("String field doesnt have a string value!");
                }
            }

            if (m_post_load.has_value()) {
                if (false == m_post_load.value()(*this, m_value.value())) {
                    SERROR_LOCAL_T("Field \"{}\" failed post load!", this->path_name().c_str());
                    throw std::runtime_error("String field failed post load!");
                }
            }

            m_is_default = false;
        } else {
            if (m_required) {
                SERROR_LOCAL_T("String field \"{}\" is required!", this->path_name().c_str());
                throw std::runtime_error("Missing required string field!");
            }

            if (m_default.has_value()) {
                m_value      = m_default.value();
                m_is_default = true;
            } else {
                SERROR_LOCAL_T("Non required string field \"{}\" has no default value!", this->path_name().c_str());
                throw std::runtime_error("Missing default value for required string field!");
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
                        SERROR_LOCAL_T("Invalid default value({}) for string field\"{}\"!", m_value.value().c_str(), this->path_name().c_str());
                        throw std::runtime_error("StringField<T> Invalid default value");
                    } else {
                        SERROR_LOCAL_T("Invalid value({}) for string field\"{}\"!", m_value.value().c_str(), this->path_name().c_str());
                        throw std::runtime_error("StringField<T> Invalid value");
                    }
                }
            }
        }
    }

    void submit(_TargetConfig& f_config) override {
        SKL_ASSERT(m_value.has_value());
        if constexpr (__is_same(std::string, _Type)) {
            f_config.*m_member_ptr = m_value.value();
        } else {
            SKL_ASSERT(m_buffer_size > 1U);
            if ((false == m_truncate_to_buffer) && ((m_buffer_size - 1U) < m_value->length())) {
                SERROR_LOCAL_T("StringField<char[{}]> \"{}\" value read overruns the target buffer!\n\tvalue->\"{}\"", m_buffer_size, this->path_name().c_str(), skl_string_view::from_std(std::string_view{m_value.value()}));
                throw std::runtime_error("StringField<char[N]> value read overruns the target buffer!");
            }

            const auto length = std::string_view{m_value.value()}.copy(f_config.*m_member_ptr, m_buffer_size - 1U);

            (f_config.*m_member_ptr)[length]             = 0;
            (f_config.*m_member_ptr)[m_buffer_size - 1U] = 0;
        }
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
        return std::make_unique<StringField<_Type, _TargetConfig>>(*this);
    }

private:
    std::optional<std::string> m_value;
    std::optional<std::string> m_default;
    member_ptr_t               m_member_ptr;
    constraints_t              m_constraints;
    std::optional<post_load_t> m_post_load;
    u64                        m_buffer_size{0ULL};
    bool                       m_required{false};
    bool                       m_validate_if_default{true};
    bool                       m_is_default{false};
    bool                       m_is_validation_only{false};
    bool                       m_truncate_to_buffer{false};
    bool                       m_dump_if_not_string{false};

    friend ConfigNode<_TargetConfig>;

    template <CPrimitiveValueFieldType, CConfigTargetType, config::CContainerType>
    friend class PrimitiveArrayField;
};
} // namespace skl::config

#undef SKL_LOG_TAG
