#include <skl_config>

using namespace skl;

struct MyChildConfig {
    struct Inner {
        float       field_float;
        std::string field_str;
    };

    float       field_float;
    std::string field_str;
    Inner       field_inner;
};

struct MyConfigRoot {
    u8            field_u8;
    i32           field_int;
    float         field_float;
    double        field_double;
    std::string   field_str;
    MyChildConfig field_obj;
    MyChildConfig field_obj2;
};

i32 main(i32 f_argc, const char** f_argv) {
    if (f_argc < 2) {
        puts("please provide the workbench.json as the only <arg>!");
        return -1;
    }

    const char* json_path = f_argv[1U];
    puts(json_path);

    ConfigNode<MyConfigRoot> root;

    root.value<u8>("u8", &MyConfigRoot::field_u8)
        .default_value(23);

    root.value<i32>("i32", &MyConfigRoot::field_int)
        .default_value(-501)
        .min_max(-500, 500);

    root.value<float>("float", &MyConfigRoot::field_float);

    root.value<std::string>("str2", &MyConfigRoot::field_str)
        .default_value("[default]")
        .min_max_length(1, 23);

    root.value<double>("double", &MyConfigRoot::field_double);

    auto child_config = ConfigNode<MyChildConfig>();

    child_config.value<float>("float", &MyChildConfig::field_float)
        .default_value(-1.245f);

    child_config.value<std::string>("string", &MyChildConfig::field_str)
        .default_value("--str--")
        .add_constraint([](auto& self, const std::string& f_value) static noexcept -> bool {
        if (f_value != "--str--") {
            SERROR_LOCAL("Field {} must be \"--str--\"!", self.path_name().c_str());
            return false;
        }

        return true;
    });

    {
        auto inner_config = ConfigNode<MyChildConfig::Inner>();
        inner_config.value<float>("float", &MyChildConfig::Inner::field_float)
            .default_value(-12.245f);

        inner_config.value<std::string>("string", &MyChildConfig::Inner::field_str)
            .default_value("--str--")
            .add_constraint([](auto& self, const std::string& f_value) static noexcept -> bool {
            if (f_value != "--str--") {
                SERROR_LOCAL("Field {} must be \"--str--\"!", self.path_name().c_str());
                return false;
            }

            return true;
        });

        child_config.object<MyChildConfig::Inner>("inner_obj", &MyChildConfig::field_inner, std::move(inner_config))
            .required(true);
    }

    root.object("obj", &MyConfigRoot::field_obj, child_config)
        .required(true);

    root.object("obj2", &MyConfigRoot::field_obj2, std::move(child_config))
        .required(true);

    MyConfigRoot config{};

    config.field_double = 12124;

    try {
        root.load_validate_and_submit(skl_string_view::from_cstr(json_path), config);
    } catch (const std::exception& f_ex) {
        (void)printf("Failed to load config from workbench.json!\n\terr-> %s\n", f_ex.what());
    }

    return 0;
}
