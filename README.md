# Skylake Config library

A fluent‐builder JSON library for loading and validating configuration

# Design points
- Main goal is at the end of the load and validation the user will have a POD filled with
  valid data, the loading, parsing and validation logic must be encapsulated only in the user's
  config module, the rest of the code must be able to use the config from eg. POD objects
- The final config objects will not use any of the types in this library, 100% decoupling
- The library must expose a builder like api allowing the user
  to build extensive and declarative configuration loding and validation logic
# Load and Validate
- The config is loaded and validated from json into raw c++ objects in **3 steps**:
    1. The fields are loaded from json using the nlohmann::json library
        - here an initial set of validations are made
            - For integral/float types, the field value is validated (that it is a propper integer/float and within the integer/float value bounds)
            - For objects, the value of the field is checked to be a json object
            - Here the required fields are checked to be present
            - Also for non required fields, default values are required and so, they are validated during the loading process 
        - if a default value is present and the field is not found (and required) in the json, the default value is used
    2. A set of "constraints" are ran against each field's value
        - These are user provided constraints eg. `min_max(0, 15)` or custom constraint logic via lambda`add_constraint([](auto& f_self, auto& f_value) static -> bool {...})`
        - **important**: The value the constraints are ran against, can be the default value
            - The default value is validated (if used)
    3. Finally, after all fields and subfields are loaded and validated, the valid values are copied into the given c++ object
        - **important**: The target C++ object is not modified if any part of the config is not valid
# Example
```cpp
#include <skl_config>

using namespace skl;

struct Inner {
    float       field_float;
    std::string field_str;
};

struct MyChildConfig {

    float             field_float;
    std::string       field_str;
    skl_vector<Inner> field_inner;
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
    //...

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
        auto inner_config = ConfigNode<Inner>();
        inner_config.value<float>("float", &Inner::field_float)
            .default_value(-12.245f);

        inner_config.value<std::string>("string", &Inner::field_str)
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

    MyConfigRoot config{};

    try {
        root.load_validate_and_submit(skl_string_view::from_cstr(json_path), config);
    } catch (const std::exception& f_ex) {
        (void)printf("Failed to load config from workbench.json!\n\terr-> %s\n", f_ex.what());
    }

    return 0;
}
```

# Validate only
- The same ConfigNode<T> can be used to validate your, in memory, T config objects
```cpp
ConfigNode<T> config{};
config.value<std::string>("str_field", &T::str_field)
    .default_value("[default_value]");
//...

//We can use the config object to validate instances of T

T config_obj{};
//Setup..
try{
    config.validate_only(config_obj);
}catch(const std::exception& f_ex){}

```

# Error reporting
- The config library produces, very descriptive error messages.

```shell
[ERROR  ][...] -- Field "__root__:u8" has an invalid integer value("asd")! Min[0] Max[255]
[ERROR  ][...] -- Field "__root__:float" has an invalid float value("Asd")! Min[1.1754944e-38] Max[3.4028235e+38]
[ERROR  ][...] -- Field "__root__:double" has an invalid double value("Asdas")! Min[2.2250738585072014e-308] Max[1.7976931348623157e+308]
[ERROR  ][...] -- Field "__root__:obj2:float" has an invalid float value("22.22,")! Min[1.1754944e-38] Max[3.4028235e+38]
[ERROR  ][...] -- Field "__root__:obj2:inner_obj:float" has an invalid float value("asdasdasd")! Min[1.1754944e-38] Max[3.4028235e+38]
[ERROR  ][...] -- Array field "__root__:obj2:inner_obj" elements count must be in [min=1, max=4294967295]!
Failed to load config from workbench.json!
```
- Here the **`__root__`** represents the json root unnamed object ```{ ... }```.
