#include <skl_config>
#include <skl_socket>
#include <skl_vector_if>
#include <skl_fixed_vector_if>

using namespace skl;

enum EMyNonClassEnum : i32 {
    EMyNonClassEnum_Value1 = -1,
    EMyNonClassEnum_Value2 = 0,
    EMyNonClassEnum_Value3,
    EMyNonClassEnum_MAX
};

enum class EMyEnum : i32 {
    Value1 = -1,
    Value2 = 0,
    Value3,
    MAX
};

struct Inner {
    float       field_float;
    std::string field_str;
};

struct Inner2 {
    std::string field_str;
};

struct MyChildConfig {

    float              field_float;
    std::string        field_str;
    std::vector<Inner> field_inner;
};

struct MyConfigRoot {
    u8                           field_u8;
    i32                          field_int;
    float                        field_float;
    double                       field_double;
    ipv4_addr_t                  field_ip_addr;
    bool                         field_bool;
    EMyNonClassEnum              field_enum;
    EMyEnum                      field_class_enum;
    char                         field_str[256U];
    MyChildConfig                field_obj;
    MyChildConfig                field_obj2;
    char                         field_buffer[8U]{0};
    std::vector<u32>             field_prim_list;
    skl_fixed_vector<u32, 4U>    field_prim_list_fixed;
    std::vector<Inner>           field_inner_via_proxy;
    skl_fixed_vector<Inner, 3U>  field_inner_via_proxy_fixed;
    skl_vector<Inner2>           field_inner2;
    skl_fixed_vector<Inner2, 4U> field_inner2_fixed;
    skl_fixed_vector<u32, 4U>    field_number_fixed;
};

namespace {
ConfigNode<MyConfigRoot>& example_get_config_loader() noexcept {
    //! Recommended: move to heap, this way it can be freed when not needed
    static ConfigNode<MyConfigRoot> root;
    static bool                     built = false;

    if (built) {
        return root;
    }

    root.numeric("ip_addr", &MyConfigRoot::field_ip_addr)
        .required(true)
        .parse_raw<decltype([](config::Field& f_self, std::string_view f_string) static -> std::optional<double> {
            const auto result = ipv4_addr_from_str(skl_string_view::from_std(f_string).data());
            if (result == CIpAny) {
                SERROR_LOCAL("Field \"{}\" must be a non-zero valid ip address!", f_self.path_name().c_str());
                return std::nullopt;
            }

            return result;
        })>();

    root.numeric("u8", &MyConfigRoot::field_u8)
        .default_value(23);

    root.numeric("i32", &MyConfigRoot::field_int)
        .default_value(-501)
        .power_of_2()
        .min(-500)
        .max(500);

    root.numeric("float", &MyConfigRoot::field_float);

    root.string("str2", &MyConfigRoot::field_str)
        .default_value("[default]")
        .dump_if_not_string(true);

    root.numeric("double", &MyConfigRoot::field_double)
        .parse_raw<decltype([](config::Field&, const std::string& f_string) static -> std::optional<double> {
            return config::NumericField<double, MyConfigRoot>::safely_convert_to_numeric(f_string);
        })>();

    root.numeric("ip_addr2", &MyConfigRoot::field_ip_addr)
        .required(true)
        .parse_json<decltype([](config::Field& f_self, json& f_json) static -> std::optional<double> {
            const auto result = ipv4_addr_from_str(f_json.get<std::string>().c_str());
            if (result == CIpAny) {
                SERROR_LOCAL("Field \"{}\" must be a non-zero valid ip address!", f_self.path_name().c_str());
                return std::nullopt;
            }

            return result;
        })>();

    root.string("str3", &MyConfigRoot::field_buffer)
        .min_length(4U)
        .truncate_to_buffer(true)
        .default_value("asdas2");

    root.boolean("bool", &MyConfigRoot::field_bool)
        .interpret_str(true)
        .interpret_str_true_value("TRUE");

    root.enumeration("enum", &MyConfigRoot::field_enum)
        .required(true);

    root.enumeration("enum2", &MyConfigRoot::field_class_enum)
        .required(true);

    auto child_config = ConfigNode<MyChildConfig>();

    child_config.numeric("float", &MyChildConfig::field_float)
        .default_value(-1.245f);

    child_config.string("string", &MyChildConfig::field_str)
        .default_value("--str--")
        .add_constraint<decltype([](config::Field& f_self, const std::string& f_value) static -> bool {
            if (f_value != "--str--") {
                SERROR_LOCAL("Field {} must be \"--str--\"!", f_self.path_name().c_str());
                return false;
            }

            return true;
        })>();

    {
        auto inner_config = ConfigNode<Inner>();
        inner_config.numeric("float", &Inner::field_float)
            .default_value(-12.245f);

        inner_config.string("string", &Inner::field_str)
            .default_value("--str--");

        child_config.array<Inner>("inner_obj", &MyChildConfig::field_inner, std::move(inner_config))
            .min_length(1U)
            .default_value({
                Inner{.field_float = 1},
                Inner{.field_float = 2},
                Inner{.field_float = 3},
                Inner{.field_float = 4},
                Inner{.field_float = 5},
            });
    }

    root.object("obj", &MyConfigRoot::field_obj, child_config)
        .required(true);

    root.object("obj2", &MyConfigRoot::field_obj2, std::move(child_config))
        .required(true);

    root.array_raw<u32>(skl_string_view::exact_cstr("field_prim_list"), &MyConfigRoot::field_prim_list)
        .required(true)
        .field()
        .min(4U)
        .power_of_2();

    root.array_raw<u32, decltype(MyConfigRoot::field_prim_list_fixed)>(skl_string_view::exact_cstr("field_prim_list_fixed"), &MyConfigRoot::field_prim_list_fixed)
        .required(true)
        .min_length(1U)
        .field()
        .min(4U)
        .power_of_2();
    {
        struct InnerProxy {
            char float_str[32U];
            u32  field_u32;

            void submit(config::Field&, Inner& f_inner) const noexcept {
                f_inner.field_float = strtof(float_str, nullptr);
                f_inner.field_str   = std::to_string(field_u32) + "_integer";
            }

            bool load(config::Field& f_self, const Inner& f_inner) noexcept {
                const auto temp = std::to_string(f_inner.field_float);
                if (temp.length() >= std::size(float_str)) {
                    SERROR_LOCAL("[InnerPorxy] Field \"{}\" -> cannot fit string (\"{}\") into InnerProxy::float_str[{}]!",
                                 f_self.path_name().c_str(),
                                 temp.c_str(),
                                 std::size(float_str));
                    return false;
                }

                field_u32 = strtoul(f_inner.field_str.c_str(), nullptr, 10);

                return true;
            }
        };

        ConfigNode<InnerProxy> inner_proxy_loader{};

        inner_proxy_loader.string("float_str", &InnerProxy::float_str)
            .required(true);

        inner_proxy_loader.numeric("field_u32", &InnerProxy::field_u32)
            .min(10U)
            .max(1024U)
            .default_value(123U);

        root.array_proxy<Inner, InnerProxy>("field_inner_via_proxy",
                                            &MyConfigRoot::field_inner_via_proxy,
                                            inner_proxy_loader)
            .required(true)
            .min_length(1U);

        root.array_proxy<Inner, InnerProxy, decltype(MyConfigRoot::field_inner_via_proxy_fixed)>("field_inner_via_proxy_fixed",
                                                                                                 &MyConfigRoot::field_inner_via_proxy_fixed,
                                                                                                 std::move(inner_proxy_loader))
            .required(true)
            .min_length(1U);
    }

    {
        ConfigNode<Inner2> inner2_loader{};

        inner2_loader.string("field_str", &Inner2::field_str)
            .default_value("default");

        root.array<Inner2, skl_vector<Inner2>>("inner2",
                                               &MyConfigRoot::field_inner2,
                                               inner2_loader)
            .default_value({Inner2{"asdasd"}, Inner2{"1111111"}});

        root.array<Inner2, decltype(MyConfigRoot::field_inner2_fixed)>("inner2_fixed",
                                                                       &MyConfigRoot::field_inner2_fixed,
                                                                       std::move(inner2_loader))
            .min_length(1U)
            .required(true);
    }

    root.post_submit<decltype([](MyConfigRoot& f_config) static {
        if (f_config.field_bool) {
            f_config.field_int += 255;
        }
        return true;
    })>();

    built = true;

    return root;
}

void example__load_from_json(const char* f_json_file_path) {
    puts(f_json_file_path);

    MyConfigRoot config{};

    try {
        auto& root = example_get_config_loader();

        root.load_validate_and_submit(skl_string_view::from_cstr(f_json_file_path), config);
    } catch (const std::exception& f_ex) {
        (void)printf("Failed to load config from workbench.json!\n\terr-> %s\n", f_ex.what());
    }
}

void example_validate_existing_config() {
    MyConfigRoot config{};

    config.field_u8     = 55U;
    config.field_int    = 32;
    config.field_float  = 12.01f;
    config.field_double = 15.01;

    (void)strcpy(config.field_buffer, "121525");

    config.field_obj.field_str  = "--str--";
    config.field_obj2.field_str = "--str--";

    config.field_obj.field_inner.push_back({.field_str = "--str--"});
    config.field_obj2.field_inner.push_back({.field_str = "--str--"});

    config.field_inner_via_proxy.push_back(Inner{.field_float = 23.2f, .field_str = "123"});
    SKL_ASSERT_PERMANENT(config.field_inner_via_proxy_fixed.upgrade().push_back(Inner{.field_float = 23.2f, .field_str = "123"}));
    SKL_ASSERT_PERMANENT(config.field_inner2_fixed.upgrade().push_back({}));
    SKL_ASSERT_PERMANENT(config.field_prim_list_fixed.upgrade().push_back(4));

    try {
        auto& root = example_get_config_loader();

        root.validate_only(config);
    } catch (const std::exception& f_ex) {
        (void)printf("Failed to validate config object!\n");
    }
}
} // namespace

i32 main(i32 f_argc, const char** f_argv) {
    if (f_argc >= 2) {
        example__load_from_json(f_argv[1U]);
    }

    example_validate_existing_config();

    return 0;
}
