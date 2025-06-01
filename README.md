# Skylake Config library

A fluent‐builder JSON configuration library

# Design points
- Main goal is at the end of the load and validation the user will have a POD filled with
  valid data, the loading, parsing and validation logic must be encapsulated only in the user's
  config module, the rest of the code must be able to use the config from eg. POD objects
- The final config objects will not use any of the types in this library, 100% decoupling
- The library must expose a builder like api allowing the user
  to build extensive and declarative configuration loding and validation logic


```cpp
{
    struct MyConfigRoot {
        u8                        field_u8;
        i32                       field_int;
        float                     field_float;
        double                    field_double;
        std::string               field_str;
        skl_fixed_vector<i32, 32> field_list_i32;
    };

    //...

    ConfigNode<MyConfigRoot> root;

    root.value<u8>("u82", &MyConfigRoot::field_u8)
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
}
```
