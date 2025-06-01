#include <skl_fixed_vector>
#include <skl_config>

using namespace skl;

struct MyConfigRoot {
    u8                        field_u8;
    i32                       field_int;
    float                     field_float;
    double                    field_double;
    std::string               field_str;
    skl_fixed_vector<i32, 32> fied_list;
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

    MyConfigRoot config{};

    try {
        root.load_validate_and_submit(skl_string_view::from_cstr(json_path), config);
    } catch (const std::exception& f_ex) {
        (void)printf("Failed to load config from workbench.json!\n\terr-> %s\n", f_ex.what());
    }

    return 0;
}
