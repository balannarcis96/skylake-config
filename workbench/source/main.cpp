#include <skl_config>

using namespace skl;

struct Inner {
    float       field_float;
    std::string field_str;
};

struct MyChildConfig {

    float              field_float;
    std::string        field_str;
    std::vector<Inner> field_inner;
};

struct MyConfigRoot {
    u8            field_u8;
    i32           field_int;
    float         field_float;
    double        field_double;
    std::string   field_str;
    MyChildConfig field_obj;
    MyChildConfig field_obj2;
    char          field_buffer[8U]{0};
};

namespace {
ConfigNode<MyConfigRoot>& example_get_config_loader() noexcept {
    //! Recommended: move to heap, this way it can be freed when not needed
    static ConfigNode<MyConfigRoot> root;
    static bool                     built = false;

    if (built) {
        return root;
    }

    root.numeric<u8>("u8", &MyConfigRoot::field_u8)
        .default_value(23);

    root.numeric<i32>("i32", &MyConfigRoot::field_int)
        .default_value(-501)
        .power_of_2()
        .min(-500)
        .max(500);

    root.numeric<float>("float", &MyConfigRoot::field_float);

    root.string("str2", &MyConfigRoot::field_str)
        .default_value("[default]")
        .min_length(1)
        .max_length(23);

    root.numeric<double>("double", &MyConfigRoot::field_double);

    root.string("str3", &MyConfigRoot::field_buffer)
        .min_length(4U)
        .truncate_to_buffer(true)
        .default_value("asdas2");

    auto child_config = ConfigNode<MyChildConfig>();

    child_config.numeric<float>("float", &MyChildConfig::field_float)
        .default_value(-1.245f);

    child_config.string("string", &MyChildConfig::field_str)
        .default_value("--str--")
        .add_constraint([](auto& self, std::string_view f_value) static noexcept -> bool {
        if (f_value != "--str--") {
            SERROR_LOCAL("Field {} must be \"--str--\"!", self.path_name().c_str());
            return false;
        }

        return true;
    });

    {
        auto inner_config = ConfigNode<Inner>();
        inner_config.numeric<float>("float", &Inner::field_float)
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
    config.field_str    = "--str--";

    (void)strcpy(config.field_buffer, "121525");

    config.field_obj.field_str  = "--str--";
    config.field_obj2.field_str = "--str--";

    config.field_obj.field_inner.push_back({.field_str = "--str--"});
    config.field_obj2.field_inner.push_back({.field_str = "--str--"});

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
