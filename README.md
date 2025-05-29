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
    ConfigBuilder<MyConfig> root{};
    ConfigBuilder<MySubConfig> sub{};

    builder.field<i32>("json_filed_name")
        .required(true)
        .add_constraint<MinMaxConstraint>(0, 500)
        .add_constraint<PowerOf2>(23)
        .bind(&MySubConfig::field);

    builder.field<i32>("json_optional_filed_name")
        .required(false)
        .default(0)
        .add_constraint<MinMaxConstraint>(0, 500)
        .add_constraint<PowerOf2>(23)
        .bind(&MySubConfig::field2);

    root.field<std::string>("json_field_name")
        .add_constraint<Predicate>([](json& field) -> bool {
            //custom validation logic
        })
        .bind_custom([](const std::string& value, MyConfig& config) {
            config.my_field = value + "_test_";
        });

    root.array(sub, "json_array_field_name")
        .required(true)
        .bind(&MySubConfig::vector_like_field);

    //1. First build
    const auto result = root.build();

    //2. Validate and store
    MyConfig config{};
    const auto result = root.read_json("path");
    const auto result = root.validate_and_store(config);
}
```
