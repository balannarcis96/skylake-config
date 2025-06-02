//!
//! \file value_field
//!
//! \license Licensed under the MIT License. See LICENSE for details.
//!
#pragma once

#include <charconv>

#include <skl_log>

#include "skl_config_internal/field.hpp"

#define SKL_LOG_TAG ""

namespace skl::config {
template <CValueFieldType _Type, CConfigTargetType _TargetConfig>
class ValueField : public ConfigField<_TargetConfig> {
public:
    using member_ptr_t  = _Type _TargetConfig::*;
    using constraints_t = std::vector<std::function<bool(ValueField<_Type, _TargetConfig>&, const _Type&)>>;

    ValueField(Field* f_parent, std::string_view f_field_name, member_ptr_t f_member_ptr) noexcept
        : ConfigField<_TargetConfig>(f_parent, f_field_name)
        , m_member_ptr(f_member_ptr) {
        add_default_constraints();
    }

    ~ValueField() override                       = default;
    ValueField(const ValueField&)                = default;
    ValueField& operator=(const ValueField&)     = default;
    ValueField(ValueField&&) noexcept            = default;
    ValueField& operator=(ValueField&&) noexcept = default;

    ValueField& default_value(_Type f_default) noexcept
        requires(__is_trivially_copyable(_Type))
    {
        m_default             = f_default;
        m_validate_if_default = true;
        return *this;
    }

    ValueField& default_value(_Type f_default, bool f_validate) noexcept
        requires(__is_trivially_copyable(_Type))
    {
        m_default             = f_default;
        m_validate_if_default = f_validate;
        return *this;
    }

    ValueField& default_value(_Type&& default_val)
        requires(false == __is_trivially_copyable(_Type))
    {
        m_default             = std::move(default_val);
        m_validate_if_default = true;
        return *this;
    }

    ValueField& default_value(_Type&& default_val, bool f_validate)
        requires(false == __is_trivially_copyable(_Type))
    {
        m_default             = std::move(default_val);
        m_validate_if_default = f_validate;
        return *this;
    }

    ValueField& required(bool f_required) noexcept {
        m_required = f_required;
        return *this;
    }

    ValueField& min_max(_Type f_min, _Type f_max) noexcept
        requires(CIntegerValueFieldType<_Type>)
    {
        add_constraint([f_min, f_max](auto& f_self, _Type f_value) {
            if ((f_value < f_min) || (f_value > f_max)) {
                SERROR("Invalid field \"{}\" value! Min[{}] Max[{}]!", f_self.name_cstr(), f_min, f_max);
                return false;
            }
            return true;
        });
        return *this;
    }

    ValueField& min_max_length(u32 f_min_length, u32 f_max_length) noexcept
        requires(__is_same(_Type, std::string))
    {
        add_constraint([f_min_length, f_max_length](auto& f_self, _Type f_value) {
            if ((f_value.length() < f_min_length) || (f_value.length() > f_max_length)) {
                SERROR("Invalid string field \"{}\" value length! Min[{}] Max[{}]!", f_self.name_cstr(), f_min_length, f_max_length);
                return false;
            }
            return true;
        });
        return *this;
    }

    template <typename _Functor>
    void add_constraint(_Functor&& f_functor) noexcept {
        m_constraints.emplace_back(std::forward<_Functor&&>(f_functor));
    }

protected:
    void load(json& f_json) override {
        const auto exists = f_json.contains(this->name());
        if (exists) {
            if constexpr (CIntegerValueFieldType<_Type>) {
                const auto result = safely_convert_to_integer(f_json[this->name()].dump());
                if (false == result.has_value()) {
                    SERROR_LOCAL_T("Field \"{}\" has an invalid {} value({})! Min[{}] Max[{}]",
                                   this->path_name().c_str(),
                                   (false == __is_same(_Type, float)) ? (__is_same(_Type, double) ? "double" : "integer") : "float",
                                   f_json[this->name()].dump().c_str(),
                                   std::numeric_limits<_Type>::min(),
                                   std::numeric_limits<_Type>::max());
                    throw std::runtime_error("Invalid integer value field!");
                } else {
                    m_value = result.value();
                }
            } else {
                m_value = f_json[this->name()].template get<_Type>();
            }
            m_is_default = false;
        } else {
            if (m_required) {
                SERROR_LOCAL_T("Field \"{}\" is required!", this->path_name().c_str());
                throw std::runtime_error("Missing required field!");
            }

            if (m_default.has_value()) {
                m_value      = m_default.value();
                m_is_default = true;
            } else {
                SERROR_LOCAL_T("Non required field \"{}\" has no default value!", this->path_name().c_str());
                throw std::runtime_error("Missing default value for required field!");
            }
        }
    }

    void validate() override {
        if (false == m_value.has_value()) {
            SKL_ASSERT((false == m_required) && (false == m_default.has_value()));
            return;
        }

        if ((false == m_is_default) || m_validate_if_default) {
            //Run constraints
            for (const auto& constraint : m_constraints) {
                if (false == constraint(*this, m_value.value())) {
                    if (m_is_default) {
                        std::string message = "Invalid default value(";
                        if constexpr (__is_same(_Type, std::string)) {
                            message += m_value.value();
                        } else {
                            message += std::to_string(m_default.value());
                        }
                        message += ") for field \"";
                        message += this->path_name();
                        message += "\"!";
                        throw std::runtime_error(message);
                    } else {
                        std::string message  = "Invalid field \"";
                        message             += this->path_name();
                        message             += "\"!";
                        throw std::runtime_error(message);
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
        m_value      = f_config.*m_member_ptr;
        m_is_default = true;
    }

    std::unique_ptr<ConfigField<_TargetConfig>> clone() override {
        return std::make_unique<ValueField<_Type, _TargetConfig>>(*this);
    }

private:
    [[nodiscard]] static std::optional<_Type> safely_convert_to_integer(std::string_view f_str)
        requires(CIntegerValueFieldType<_Type>)
    {
        _Type value;
        auto [ptr, ec] = std::from_chars(f_str.data(), f_str.data() + f_str.size(), value);
        if (ec != std::errc{}) {
            return std::nullopt;
        }

        return value;
    }

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

    friend ConfigNode<_TargetConfig>;
};
} // namespace skl::config

#undef SKL_LOG_TAG
