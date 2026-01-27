//======================================================================
// Skylake Config Library - Comprehensive Workbench Example
//======================================================================
// This workbench demonstrates all major features of the Skylake Config library:
// - Numeric fields (integers, floats, doubles) with constraints
// - String fields (std::string and fixed char buffers)
// - Boolean fields with custom interpretation
// - Enum fields (both scoped and unscoped)
// - Nested object fields
// - Array fields (objects and primitives)
// - Array proxy fields (type transformation)
// - Custom parsers and validators
// - Validation-only mode
// - Post-submit hooks

#include <skl_config>
#include <skl_assert>
#include <skl_socket>
#include <skl_vector_if>
#include <skl_fixed_vector_if>
#include <cstdio>

using namespace skl;

//======================================================================
// Configuration Data Structures
//======================================================================

// Enum: Network protocol types (unscoped enum)
enum EMyNonClassEnum : i32 {
    EMyNonClassEnum_Value1 = -1,
    EMyNonClassEnum_Value2 = 0,
    EMyNonClassEnum_Value3,
    EMyNonClassEnum_MAX
};

// Enum: Log levels (scoped enum - preferred)
enum class EMyEnum : i32 {
    Value1 = -1,
    Value2 = 0,
    Value3,
    MAX
};

// Inner configuration object (for nested arrays)
struct Inner {
    float       field_float;
    std::string field_str;
};

// Simple inner object for testing ATRP containers
struct Inner2 {
    std::string field_str;
};

// Child configuration object (for nested object demonstration)
struct MyChildConfig {
    float              field_float;
    std::string        field_str;
    std::vector<Inner> field_inner;
};

// Root configuration structure
struct MyConfigRoot {
    // Basic numeric types
    u8     field_u8;
    i32    field_int;
    float  field_float;
    double field_double;

    // IPv4 address stored as double (demonstrates custom parser)
    ipv4_addr_t field_ip_addr;

    // Boolean and enum fields
    bool            field_bool;
    EMyNonClassEnum field_enum;
    EMyEnum         field_class_enum;

    // String fields (fixed-size buffer and dynamic)
    char field_str[256U];
    char field_buffer[8U]{0};

    // Nested objects
    MyChildConfig field_obj;
    MyChildConfig field_obj2;

    // Primitive arrays (resizable and fixed-size)
    std::vector<u32>          field_prim_list;
    skl_fixed_vector<u32, 4U> field_prim_list_fixed;

    // Object arrays via proxy (type transformation during load)
    std::vector<Inner>          field_inner_via_proxy;
    skl_fixed_vector<Inner, 3U> field_inner_via_proxy_fixed;

    // Object arrays with ATRP containers
    skl_vector<Inner2>           field_inner2;
    skl_fixed_vector<Inner2, 4U> field_inner2_fixed;

    // Additional fixed-size numeric array
    skl_fixed_vector<u32, 4U> field_number_fixed;
};

//======================================================================
// Configuration Loader Builder
//======================================================================

namespace {

ConfigNode<MyConfigRoot>& get_config_loader() noexcept {
    static ConfigNode<MyConfigRoot> root;
    static bool                     initialized = false;

    if (initialized) {
        return root;
    }

    //------------------------------------------------------------------
    // Basic Numeric Fields with Constraints
    //------------------------------------------------------------------

    // U8 field with default value
    root.numeric<u8>("u8", &MyConfigRoot::field_u8)
        .default_value(23);

    // I32 field with power-of-2 constraint and range
    // Note: default -501 is outside the min/max range, but power_of_2() allows it
    root.numeric<i32>("i32", &MyConfigRoot::field_int)
        .default_value(-501)
        .power_of_2()
        .min(-500)
        .max(500);

    // Float field (required, no default)
    root.numeric<float>("float", &MyConfigRoot::field_float);

    // Double field with custom raw string parser
    root.numeric<double>("double", &MyConfigRoot::field_double)
        .parse_raw<decltype([](config::Field&, const std::string& f_string) static -> std::optional<double> {
            return config::NumericField<double, MyConfigRoot>::safely_convert_to_numeric(f_string);
        })>();

    //------------------------------------------------------------------
    // Custom Parser: IPv4 Address (String -> Double conversion)
    //------------------------------------------------------------------

    // Parse IPv4 address from string format "192.168.0.1"
    root.numeric<ipv4_addr_t>("ip_addr", &MyConfigRoot::field_ip_addr)
        .required(true)
        .parse_raw<decltype([](config::Field& f_self, std::string_view f_string) static -> std::optional<double> {
            const auto result = ipv4_addr_from_str(skl_string_view::from_std(f_string).data());
            if (result == CIpAny) {
                SERROR_LOCAL("Field \"{}\" must be a non-zero valid ip address!", f_self.path_name().c_str());
                return std::nullopt;
            }
            return result;
        })>();

    // Alternative: Parse IPv4 from JSON string using JSON parser
    root.numeric<ipv4_addr_t>("ip_addr2", &MyConfigRoot::field_ip_addr)
        .required(true)
        .parse_json<decltype([](config::Field& f_self, json& f_json) static -> std::optional<double> {
            const auto result = ipv4_addr_from_str(f_json.get<std::string>().c_str());
            if (result == CIpAny) {
                SERROR_LOCAL("Field \"{}\" must be a non-zero valid ip address!", f_self.path_name().c_str());
                return std::nullopt;
            }
            return result;
        })>();

    //------------------------------------------------------------------
    // String Fields
    //------------------------------------------------------------------

    // Dynamic string with default, allows non-string JSON (converts to string)
    root.string("str2", &MyConfigRoot::field_str)
        .default_value("[default]")
        .dump_if_not_string(true);

    // Fixed-size buffer with length constraint and truncation
    root.string("str3", &MyConfigRoot::field_buffer)
        .min_length(4U)
        .truncate_to_buffer(true)
        .default_value("asdas2");

    //------------------------------------------------------------------
    // Boolean Field with Custom String Interpretation
    //------------------------------------------------------------------

    root.boolean("bool", &MyConfigRoot::field_bool)
        .interpret_str(true)
        .interpret_str_true_value("TRUE");

    //------------------------------------------------------------------
    // Enum Fields
    //------------------------------------------------------------------

    // Unscoped enum with exclusions
    root.enumeration("enum", &MyConfigRoot::field_enum)
        .exclude(EMyNonClassEnum::EMyNonClassEnum_Value1)
        .exclude(EMyNonClassEnum::EMyNonClassEnum_MAX)
        .required(true);

    // Scoped enum with max constraint
    root.enumeration("enum2", &MyConfigRoot::field_class_enum)
        .max(EMyEnum::MAX)
        .required(true);

    //------------------------------------------------------------------
    // Nested Object Configuration
    //------------------------------------------------------------------

    auto child_config = ConfigNode<MyChildConfig>();

    // Child object fields
    child_config.numeric<float>("float", &MyChildConfig::field_float)
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

    // Nested array within child object
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

    // Register nested objects (obj and obj2 share same configuration)
    root.object("obj", &MyConfigRoot::field_obj, child_config)
        .required(true);

    root.object("obj2", &MyConfigRoot::field_obj2, std::move(child_config))
        .required(true);

    //------------------------------------------------------------------
    // Primitive Arrays
    //------------------------------------------------------------------

    // Resizable array of u32 with element-level constraints
    root.array_raw<u32>(skl_string_view::exact_cstr("field_prim_list"), &MyConfigRoot::field_prim_list)
        .required(true)
        .field()  // Access element-level configuration
        .min(4U)
        .power_of_2();

    // Fixed-size array of u32 with element-level constraints
    root.array_raw<u32, decltype(MyConfigRoot::field_prim_list_fixed)>(skl_string_view::exact_cstr("field_prim_list_fixed"), &MyConfigRoot::field_prim_list_fixed)
        .required(true)
        .min_length(1U)
        .field()  // Access element-level configuration
        .min(4U)
        .power_of_2();

    //------------------------------------------------------------------
    // Array Via Proxy (Type Transformation)
    //------------------------------------------------------------------
    // This demonstrates loading JSON into a proxy type, then transforming
    // it into the target type during submit.

    {
        // Proxy structure for transforming JSON -> Inner
        struct InnerProxy {
            char float_str[32U];
            u32  field_u32;

            // Submit: Convert proxy -> target type
            void submit(config::Field&, Inner& f_inner) const noexcept {
                f_inner.field_float = strtof(float_str, nullptr);
                f_inner.field_str   = std::to_string(field_u32) + "_integer";
            }

            // Load: Convert target type -> proxy (for validation-only mode)
            bool load(config::Field& f_self, const Inner& f_inner) noexcept {
                const auto temp = std::to_string(f_inner.field_float);
                if (temp.length() >= std::size(float_str)) {
                    SERROR_LOCAL("[InnerProxy] Field \"{}\" -> cannot fit string (\"{}\") into InnerProxy::float_str[{}]!",
                                 f_self.path_name().c_str(),
                                 temp.c_str(),
                                 std::size(float_str));
                    return false;
                }

                std::copy(temp.begin(), temp.end(), float_str);
                float_str[temp.length()] = '\0';

                field_u32 = strtoul(f_inner.field_str.c_str(), nullptr, 10);

                return true;
            }
        };

        ConfigNode<InnerProxy> inner_proxy_loader{};

        inner_proxy_loader.string("float_str", &InnerProxy::float_str)
            .required(true);

        inner_proxy_loader.numeric<u32>("field_u32", &InnerProxy::field_u32)
            .min(10U)
            .max(1024U)
            .default_value(123U);

        // Resizable array via proxy
        root.array_proxy<Inner, InnerProxy>("field_inner_via_proxy",
                                            &MyConfigRoot::field_inner_via_proxy,
                                            inner_proxy_loader)
            .required(true)
            .min_length(1U);

        // Fixed-size array via proxy
        root.array_proxy<Inner, InnerProxy, decltype(MyConfigRoot::field_inner_via_proxy_fixed)>("field_inner_via_proxy_fixed",
                                                                                                 &MyConfigRoot::field_inner_via_proxy_fixed,
                                                                                                 std::move(inner_proxy_loader))
            .required(true)
            .min_length(1U);
    }

    //------------------------------------------------------------------
    // Object Arrays with ATRP Containers
    //------------------------------------------------------------------

    {
        ConfigNode<Inner2> inner2_loader{};

        inner2_loader.string("field_str", &Inner2::field_str)
            .default_value("default");

        // ATRP resizable vector
        root.array<Inner2, skl_vector<Inner2>>("inner2",
                                               &MyConfigRoot::field_inner2,
                                               inner2_loader)
            .default_value({Inner2{"asdasd"}, Inner2{"1111111"}});

        // ATRP fixed-size vector
        root.array<Inner2, decltype(MyConfigRoot::field_inner2_fixed)>("inner2_fixed",
                                                                       &MyConfigRoot::field_inner2_fixed,
                                                                       std::move(inner2_loader))
            .min_length(1U)
            .required(true);
    }

    //------------------------------------------------------------------
    // Post-Submit Hook
    //------------------------------------------------------------------
    // This hook is called after all validation passes and values are submitted
    // to the target object. Useful for post-processing or derived values.

    root.post_submit<decltype([](MyConfigRoot& f_config) static {
        if (f_config.field_bool) {
            f_config.field_int += 255;
        }
        return true;
    })>();

    initialized = true;
    return root;
}

//======================================================================
// Example: Load Configuration from JSON File
//======================================================================

void example_load_from_json(const char* f_json_file_path) {
    printf("========================================\n");
    printf("Example: Load Configuration from JSON\n");
    printf("========================================\n");
    printf("JSON file: %s\n\n", f_json_file_path);

    MyConfigRoot config{};

    try {
        auto& root = get_config_loader();
        root.load_validate_and_submit(skl_string_view::from_cstr(f_json_file_path), config);

        printf("✓ Configuration loaded and validated successfully!\n\n");

        // Display loaded values
        printf("Loaded Configuration Values:\n");
        printf("----------------------------\n");
        printf("  field_u8:          %u\n", config.field_u8);
        printf("  field_int:         %d\n", config.field_int);
        printf("  field_float:       %.2f\n", config.field_float);
        printf("  field_double:      %.2f\n", config.field_double);
        printf("  field_bool:        %s\n", config.field_bool ? "true" : "false");
        printf("  field_str:         %s\n", config.field_str);
        printf("  field_buffer:      %s\n", config.field_buffer);
        printf("  field_enum:        %d\n", config.field_enum);
        printf("  field_class_enum:  %d\n", static_cast<i32>(config.field_class_enum));

        printf("\n  Nested Object (obj):\n");
        printf("    field_float:     %.2f\n", config.field_obj.field_float);
        printf("    field_str:       %s\n", config.field_obj.field_str.c_str());
        printf("    field_inner:     %zu elements\n", config.field_obj.field_inner.size());

        printf("\n  Primitive Arrays:\n");
        printf("    field_prim_list: [");
        for (size_t i = 0; i < config.field_prim_list.size(); ++i) {
            printf("%u%s", config.field_prim_list[i], i < config.field_prim_list.size() - 1 ? ", " : "");
        }
        printf("]\n");

        printf("    field_prim_list_fixed: [");
        for (size_t i = 0; i < config.field_prim_list_fixed.size(); ++i) {
            printf("%u%s", config.field_prim_list_fixed[i], i < config.field_prim_list_fixed.size() - 1 ? ", " : "");
        }
        printf("]\n");

        printf("\n  Proxy Arrays:\n");
        printf("    field_inner_via_proxy: %zu elements\n", config.field_inner_via_proxy.size());
        if (!config.field_inner_via_proxy.empty()) {
            printf("      [0] float=%.2f, str=\"%s\"\n",
                   config.field_inner_via_proxy[0].field_float,
                   config.field_inner_via_proxy[0].field_str.c_str());
        }

        printf("\n  ATRP Arrays:\n");
        printf("    field_inner2_fixed: %zu elements\n", config.field_inner2_fixed.size());

        printf("\n");

    } catch (const std::exception& f_ex) {
        printf("✗ Failed to load configuration!\n");
        printf("  Error: %s\n\n", f_ex.what());
    }
}

//======================================================================
// Example: Validate Existing In-Memory Configuration
//======================================================================

void example_validate_existing_config() {
    printf("========================================\n");
    printf("Example: Validate In-Memory Config\n");
    printf("========================================\n");
    printf("Creating and validating config object in code...\n\n");

    MyConfigRoot config{};

    // Populate configuration object in code
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
    (void)config.field_inner_via_proxy_fixed.upgrade().push_back(Inner{.field_float = 23.2f, .field_str = "123"});
    (void)config.field_inner2_fixed.upgrade().push_back({});
    (void)config.field_prim_list_fixed.upgrade().push_back(4);

    try {
        auto& root = get_config_loader();
        root.validate_only(config);

        printf("✓ In-memory configuration validated successfully!\n");
        printf("  All constraints satisfied.\n\n");

    } catch (const std::exception& f_ex) {
        printf("✗ Validation failed!\n");
        printf("  Error: %s\n\n", f_ex.what());
    }
}

//======================================================================
// Example: Demonstrate Validation Failures
//======================================================================

void example_validation_failures() {
    printf("========================================\n");
    printf("Example: Validation Failure Cases\n");
    printf("========================================\n");
    printf("Demonstrating validation error messages...\n\n");

    MyConfigRoot config{};

    // Test 1: Invalid power-of-2 constraint
    printf("Test 1: Invalid power-of-2 value\n");
    config.field_u8     = 55U;
    config.field_int    = 15;  // Not a power of 2
    config.field_float  = 12.01f;
    config.field_double = 15.01;
    (void)strcpy(config.field_buffer, "121525");
    config.field_obj.field_str  = "--str--";
    config.field_obj2.field_str = "--str--";
    config.field_obj.field_inner.push_back({.field_str = "--str--"});
    config.field_obj2.field_inner.push_back({.field_str = "--str--"});

    try {
        auto& root = get_config_loader();
        root.validate_only(config);
        printf("  Unexpected: validation passed\n");
    } catch (const std::exception& f_ex) {
        printf("  Expected error: %s\n", f_ex.what());
    }

    printf("\n");

    // Test 2: Invalid constraint (string must be "--str--")
    printf("Test 2: Invalid string constraint\n");
    config.field_int           = 32;  // Fix previous error
    config.field_obj.field_str = "wrong_value";

    try {
        auto& root = get_config_loader();
        root.validate_only(config);
        printf("  Unexpected: validation passed\n");
    } catch (const std::exception& f_ex) {
        printf("  Expected error: %s\n", f_ex.what());
    }

    printf("\n");
}

} // namespace

//======================================================================
// Main Entry Point
//======================================================================

i32 main(i32 f_argc, const char** f_argv) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  Skylake Config Library - Workbench Example             ║\n");
    printf("║  Comprehensive demonstration of library features         ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    // Example 1: Load configuration from JSON file
    if (f_argc >= 2) {
        example_load_from_json(f_argv[1]);
    } else {
        printf("Note: Provide JSON file path as argument to test file loading.\n");
        printf("      Usage: %s <path_to_config.json>\n\n", f_argc > 0 ? f_argv[0] : "workbench");
    }

    // Example 2: Validate in-memory configuration
    example_validate_existing_config();

    // Example 3: Demonstrate validation failures
    example_validation_failures();

    printf("========================================\n");
    printf("Workbench Complete\n");
    printf("========================================\n");
    printf("\n");

    return 0;
}
