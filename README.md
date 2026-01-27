# Skylake Config Library - Comprehensive Guide

## Table of Contents
1. [Overview](#overview)
2. [Design Philosophy](#design-philosophy)
3. [Core Concepts](#core-concepts)
4. [Field Types](#field-types)
5. [Usage Examples](#usage-examples)
6. [Validation](#validation)
7. [Advanced Features](#advanced-features)
8. [Error Handling](#error-handling)
9. [Best Practices](#best-practices)

---

## Overview

Skylake Config is a modern C++23 fluent-builder JSON configuration library designed for Linux x86_64 with Clang. It provides type-safe, declarative configuration loading with comprehensive validation and clear error messages.

### Key Features
- **Type Safety**: Compile-time type checking using C++20/23 concepts
- **Fluent API**: Chainable builder pattern for intuitive configuration
- **Complete Decoupling**: Final POD objects contain no library types
- **Fail-Fast Validation**: Comprehensive validation before modifying target objects
- **Rich Error Messages**: Detailed error reporting with full field paths
- **Zero Dependencies**: Only requires nlohmann::json for JSON parsing

---

## Design Philosophy

### 100% Decoupling
The library guarantees complete separation between configuration logic and application code:
- Final config objects are Plain Old Data (POD) structures
- No library types leak into application code
- Configuration module encapsulates all loading/parsing/validation logic

### Three-Stage Process
Configuration loading follows a strict three-phase approach:

#### 1. Load from JSON
```cpp
// Initial validation during JSON parsing:
// - Type checking (integers, floats, strings, objects, arrays)
// - Range validation (numeric types within bounds)
// - Required field presence checking
// - Default value application for missing optional fields
```

#### 2. Validate
```cpp
// User-defined constraints execution:
// - Built-in constraints (min, max, min_length, etc.)
// - Custom constraints via lambdas
// - Default values are also validated if used
```

#### 3. Submit to POD
```cpp
// Copy validated data to target object:
// - Target object is NEVER modified if validation fails
// - Atomic all-or-nothing semantics
```

---

## Core Concepts

### ConfigNode&lt;T&gt;
The root container managing all fields for configuration type `T`:

```cpp
struct MyConfig {
    int port;
    std::string host;
};

ConfigNode<MyConfig> config;
```

### Field Registration
Fields are registered using fluent API methods:

```cpp
config.numeric<int>("port", &MyConfig::port)
    .default_value(8080)
    .min(1024)
    .max(65535);

config.string("host", &MyConfig::host)
    .default_value("localhost")
    .required(true);
```

### Load, Validate, and Submit
```cpp
MyConfig config_obj{};

try {
    config.load_validate_and_submit("/path/to/config.json", config_obj);
    // config_obj now contains validated data
} catch (const std::exception& ex) {
    // Handle validation errors
}
```

---

## Field Types

### 1. Numeric Fields

**Supported Types**: `i8`, `u8`, `i16`, `u16`, `i32`, `u32`, `i64`, `u64`, `float`, `double`

```cpp
struct Config {
    u16 port;
    i32 timeout_ms;
    float ratio;
    double precision;
};

ConfigNode<Config> root;

// Integer field with range validation
root.numeric<u16>("port", &Config::port)
    .required(true)
    .min(1024)
    .max(65535);

// Integer field with power-of-2 constraint
root.numeric<i32>("buffer_size", &Config::buffer_size)
    .default_value(4096)
    .power_of_2();  // Must be 2, 4, 8, 16, 32, ...

// Float with default value
root.numeric<float>("ratio", &Config::ratio)
    .default_value(1.5f)
    .min(0.0f)
    .max(10.0f);

// Double with custom parser
root.numeric<double>("precision", &Config::precision)
    .parse_raw<decltype([](config::Field&, std::string_view str) static -> std::optional<double> {
        // Custom parsing logic
        return std::stod(std::string(str));
    })>();
```

**Available Methods**:
- `.default_value(T value)` - Set default if field missing from JSON
- `.required(bool)` - Mark field as required
- `.min(T min_val)` - Minimum value constraint
- `.max(T max_val)` - Maximum value constraint
- `.power_of_2()` - Constrain to powers of 2 (integers only, min value is 2)
- `.parse_raw<Functor>()` - Custom string parser
- `.parse_json<Functor>()` - Custom JSON parser
- `.add_constraint<Functor>()` - Add custom validation

---

### 2. String Fields

**Supported Types**: `std::string`, `char[N]` (fixed-size buffers)

```cpp
struct Config {
    std::string name;
    char buffer[256];
};

ConfigNode<Config> root;

// String field with length constraints
root.string("name", &Config::name)
    .default_value("default_name")
    .min_length(3)
    .max_length(50);

// String field with combined min/max
root.string("description", &Config::description)
    .min_max_length(10, 500);

// Fixed-size buffer with truncation
root.string("buffer", &Config::buffer)
    .default_value("initial")
    .truncate_to_buffer(true)  // Truncate if too long
    .min_length(4);

// String with custom constraint
root.string("email", &Config::email)
    .add_constraint<decltype([](config::Field& self, const std::string& value) static -> bool {
        if (value.find('@') == std::string::npos) {
            SERROR("Field {} must be a valid email!", self.path_name().c_str());
            return false;
        }
        return true;
    })>();
```

**Available Methods**:
- `.default_value(std::string)` - Set default value
- `.required(bool)` - Mark as required
- `.min_length(size_t)` - Minimum length constraint
- `.max_length(size_t)` - Maximum length constraint
- `.min_max_length(size_t min, size_t max)` - Combined min/max
- `.truncate_to_buffer(bool)` - Truncate to buffer size (char[N] only)
- `.dump_if_not_string(bool)` - Convert JSON non-string to string representation
- `.add_constraint<Functor>()` - Add custom validation

---

### 3. Boolean Fields

**Supported Types**: `bool`, integral types interpreted as boolean

```cpp
struct Config {
    bool enabled;
    bool verbose;
};

ConfigNode<Config> root;

// Basic boolean field
root.boolean("enabled", &Config::enabled)
    .default_value(false);

// Boolean with string interpretation
root.boolean("verbose", &Config::verbose)
    .interpret_str(true)
    .interpret_str_true_value("YES")
    .interpret_str_false_value("NO");

// Boolean from numeric (0 = false, non-zero = true)
root.boolean("flag", &Config::flag)
    .interpret_numeric(true);
```

**Available Methods**:
- `.default_value(bool)` - Set default value
- `.required(bool)` - Mark as required
- `.interpret_str(bool)` - Enable string interpretation
- `.interpret_str_true_value(std::string)` - Custom true string (default: "true")
- `.interpret_str_false_value(std::string)` - Custom false string (default: "false")
- `.interpret_numeric(bool)` - Enable numeric interpretation (0=false, non-zero=true)
- `.add_constraint<Functor>()` - Add custom validation

---

### 4. Enum Fields

**Supported Types**: C++11 enums (both scoped and unscoped)

Uses `magic_enum` library for automatic string-to-enum conversion.

```cpp
enum class LogLevel : int {
    Debug = 0,
    Info,
    Warning,
    Error,
    MAX
};

enum Protocol : int {
    TCP = 0,
    UDP = 1,
    SCTP = 2
};

struct Config {
    LogLevel log_level;
    Protocol protocol;
};

ConfigNode<Config> root;

// Enum class with exclusions
root.enumeration("log_level", &Config::log_level)
    .default_value(LogLevel::Info)
    .exclude(LogLevel::MAX);

// Unscoped enum with min/max
root.enumeration("protocol", &Config::protocol)
    .required(true)
    .min(Protocol::TCP)
    .max(Protocol::UDP);  // Excludes SCTP
```

**JSON Example**:
```json
{
    "log_level": "Info",
    "protocol": "UDP"
}
```

**Available Methods**:
- `.default_value(Enum)` - Set default value
- `.required(bool)` - Mark as required
- `.min(Enum)` - Minimum allowed value
- `.max(Enum)` - Maximum allowed value
- `.exclude(Enum)` - Exclude specific enum value
- `.add_constraint<Functor>()` - Add custom validation

---

### 5. Object Fields (Nested Objects)

**Supported Types**: User-defined structs/classes

```cpp
struct DatabaseConfig {
    std::string host;
    u16 port;
};

struct AppConfig {
    DatabaseConfig database;
};

ConfigNode<AppConfig> root;

// Define nested object configuration
ConfigNode<DatabaseConfig> db_config;
db_config.string("host", &DatabaseConfig::host)
    .default_value("localhost");
db_config.numeric<u16>("port", &DatabaseConfig::port)
    .default_value(5432);

// Register nested object
root.object("database", &AppConfig::database, std::move(db_config))
    .required(true);
```

**JSON Example**:
```json
{
    "database": {
        "host": "db.example.com",
        "port": 3306
    }
}
```

**Available Methods**:
- `.required(bool)` - Mark as required
- `.validation_only(bool)` - Only validate, don't load

---

### 6. Array Fields (Object Arrays)

**Supported Types**: `std::vector<T>`, `skl_vector<T>`, `skl_fixed_vector<T, N>`

```cpp
struct Server {
    std::string host;
    u16 port;
};

struct Config {
    std::vector<Server> servers;
};

ConfigNode<Config> root;

// Define element configuration
ConfigNode<Server> server_config;
server_config.string("host", &Server::host).required(true);
server_config.numeric<u16>("port", &Server::port).required(true);

// Register array field
root.array<Server>("servers", &Config::servers, std::move(server_config))
    .min_length(1)
    .max_length(10)
    .default_value({
        Server{.host = "default.local", .port = 8080}
    });
```

**JSON Example**:
```json
{
    "servers": [
        {"host": "server1.com", "port": 8080},
        {"host": "server2.com", "port": 8081}
    ]
}
```

**Available Methods**:
- `.min_length(size_t)` - Minimum array length
- `.max_length(size_t)` - Maximum array length
- `.min_max_length(size_t min, size_t max)` - Combined min/max
- `.default_value(std::vector<T>)` - Set default array
- `.required(bool)` - Mark as required
- `.truncate_on_overflow(bool)` - Truncate if exceeds fixed capacity

---

### 7. Primitive Array Fields

**Supported Types**: Arrays of primitives (`std::vector<int>`, `std::vector<std::string>`, etc.)

```cpp
struct Config {
    std::vector<u32> allowed_ports;
    std::vector<std::string> hostnames;
    skl_fixed_vector<u32, 8> fixed_list;
};

ConfigNode<Config> root;

// Numeric array
root.array_numeric<u32>("allowed_ports", &Config::allowed_ports)
    .min_length(1)
    .max_length(100)
    .default_value({80, 443, 8080});

// String array
root.array_string("hostnames", &Config::hostnames)
    .min_length(1)
    .default_value({"localhost"});

// Fixed-size array
root.array_numeric<u32>("fixed_list", &Config::fixed_list)
    .truncate_on_overflow(true);
```

**JSON Example**:
```json
{
    "allowed_ports": [80, 443, 8080, 3000],
    "hostnames": ["api.example.com", "cdn.example.com"]
}
```

**Available Methods**:
- `.min_length(size_t)` - Minimum array length
- `.max_length(size_t)` - Maximum array length
- `.min_max_length(size_t min, size_t max)` - Combined min/max
- `.default_value(std::vector<T>)` - Set default array
- `.required(bool)` - Mark as required
- `.truncate_on_overflow(bool)` - Truncate if exceeds capacity

---

### 8. Array Via Proxy Fields

Advanced feature for transforming JSON representations to different C++ types during load.

```cpp
struct Inner {
    std::string name;
};

struct InnerProxy {
    std::string value;

    // Conversion operator
    operator Inner() const {
        return Inner{.name = value};
    }
};

struct Config {
    std::vector<Inner> items;
};

ConfigNode<Config> root;

ConfigNode<InnerProxy> proxy_config;
proxy_config.string("value", &InnerProxy::value).required(true);

root.array_via_proxy<InnerProxy, Inner>("items", &Config::items, std::move(proxy_config))
    .min_length(1);
```

---

## Usage Examples

### Complete Configuration Example

```cpp
#include <skl_config>

using namespace skl;

// Configuration structures
struct LoggingConfig {
    std::string file_path;
    u32 max_size_mb;
    u32 rotation_count;
};

struct ServerConfig {
    std::string bind_address;
    u16 port;
    u32 max_connections;
    u32 timeout_seconds;
};

struct AppConfig {
    std::string app_name;
    bool debug_mode;
    LoggingConfig logging;
    std::vector<ServerConfig> servers;
};

// Configuration builder function
ConfigNode<AppConfig>& get_config_loader() {
    static ConfigNode<AppConfig> root;
    static bool initialized = false;

    if (initialized) return root;

    // Root level fields
    root.string("app_name", &AppConfig::app_name)
        .required(true)
        .min_length(1)
        .max_length(100);

    root.boolean("debug_mode", &AppConfig::debug_mode)
        .default_value(false);

    // Nested logging configuration
    ConfigNode<LoggingConfig> logging_config;
    logging_config.string("file_path", &LoggingConfig::file_path)
        .default_value("/var/log/app.log");
    logging_config.numeric<u32>("max_size_mb", &LoggingConfig::max_size_mb)
        .default_value(100)
        .min(1)
        .max(10000);
    logging_config.numeric<u32>("rotation_count", &LoggingConfig::rotation_count)
        .default_value(5)
        .min(1)
        .max(100);

    root.object("logging", &AppConfig::logging, std::move(logging_config));

    // Server array configuration
    ConfigNode<ServerConfig> server_config;
    server_config.string("bind_address", &ServerConfig::bind_address)
        .default_value("0.0.0.0");
    server_config.numeric<u16>("port", &ServerConfig::port)
        .required(true)
        .min(1024)
        .max(65535);
    server_config.numeric<u32>("max_connections", &ServerConfig::max_connections)
        .default_value(1000)
        .min(1);
    server_config.numeric<u32>("timeout_seconds", &ServerConfig::timeout_seconds)
        .default_value(30)
        .min(1)
        .max(3600);

    root.array<ServerConfig>("servers", &AppConfig::servers, std::move(server_config))
        .required(true)
        .min_length(1)
        .max_length(10);

    initialized = true;
    return root;
}

// Usage
int main() {
    AppConfig config{};

    try {
        auto& loader = get_config_loader();
        loader.load_validate_and_submit("/etc/myapp/config.json", config);

        // Use validated config
        printf("App: %s\n", config.app_name.c_str());
        printf("Debug: %s\n", config.debug_mode ? "enabled" : "disabled");
        printf("Servers: %zu\n", config.servers.size());

    } catch (const std::exception& ex) {
        fprintf(stderr, "Config error: %s\n", ex.what());
        return 1;
    }

    return 0;
}
```

**Corresponding JSON**:
```json
{
    "app_name": "MyAwesomeApp",
    "debug_mode": true,
    "logging": {
        "file_path": "/var/log/myapp.log",
        "max_size_mb": 200,
        "rotation_count": 10
    },
    "servers": [
        {
            "bind_address": "127.0.0.1",
            "port": 8080,
            "max_connections": 5000,
            "timeout_seconds": 60
        },
        {
            "bind_address": "0.0.0.0",
            "port": 8443,
            "max_connections": 10000,
            "timeout_seconds": 120
        }
    ]
}
```

---

## Validation

### Built-in Constraints

The library provides numerous built-in constraints:

```cpp
// Numeric constraints
.min(100)
.max(1000)
.power_of_2()

// String constraints
.min_length(5)
.max_length(50)
.min_max_length(5, 50)

// Array constraints
.min_length(1)
.max_length(100)
.min_max_length(1, 100)

// Enum constraints
.min(EnumType::MinValue)
.max(EnumType::MaxValue)
.exclude(EnumType::InvalidValue)

// General
.required(true)
```

### Custom Constraints

Add custom validation logic using lambdas:

```cpp
// Custom numeric validation
root.numeric<u16>("port", &Config::port)
    .add_constraint<decltype([](config::Field& self, u16 value) static -> bool {
        if (value == 0 || value < 1024) {
            SERROR("Field {} must be >= 1024!", self.path_name().c_str());
            return false;
        }
        return true;
    })>();

// Custom string validation (regex-like)
root.string("email", &Config::email)
    .add_constraint<decltype([](config::Field& self, const std::string& value) static -> bool {
        if (value.find('@') == std::string::npos || value.find('.') == std::string::npos) {
            SERROR("Field {} must be a valid email address!", self.path_name().c_str());
            return false;
        }
        return true;
    })>();

// Multi-field validation
root.numeric<u16>("min_port", &Config::min_port);
root.numeric<u16>("max_port", &Config::max_port)
    .add_constraint<decltype([](config::Field& self, u16 max_port) static -> bool {
        // Note: You'll need to access min_port from parent context
        // This is a simplified example
        return true;
    })>();
```

### Validation-Only Mode

Use the same ConfigNode to validate in-memory objects without loading from JSON:

```cpp
ConfigNode<MyConfig> validator;
// ... configure fields ...

MyConfig config_obj{};
// ... populate config_obj in code ...

try {
    validator.validate_only(config_obj);
    // config_obj is valid
} catch (const std::exception& ex) {
    // Validation failed
}
```

---

## Advanced Features

### Custom Parsers

#### Raw String Parser
Parse field from raw JSON string representation:

```cpp
root.numeric<double>("ip_address", &Config::ip_as_double)
    .parse_raw<decltype([](config::Field& self, std::string_view str) static -> std::optional<double> {
        // Parse IPv4 address string to double representation
        auto result = parse_ipv4(str);
        if (!result.has_value()) {
            SERROR("Field {} must be valid IPv4 address!", self.path_name().c_str());
            return std::nullopt;
        }
        return result.value();
    })>();
```

#### JSON Parser
Parse field directly from nlohmann::json object:

```cpp
root.string("complex_field", &Config::field)
    .parse_json<decltype([](config::Field& self, json& j) static -> std::optional<std::string> {
        // Custom JSON parsing logic
        if (j.is_string()) {
            return j.get<std::string>();
        } else if (j.is_object()) {
            return j.dump();  // Convert object to string
        }
        return std::nullopt;
    })>();
```

### Default Values

Default values are validated just like loaded values:

```cpp
root.numeric<i32>("value", &Config::value)
    .default_value(-500)  // This will be validated!
    .min(-400)            // Error: default -500 < min -400!
    .max(400);
```

### Required Fields

Mark fields as required to enforce their presence in JSON:

```cpp
root.string("api_key", &Config::api_key)
    .required(true);  // Must be present in JSON
```

### Fixed-Size Containers

Support for fixed-capacity containers:

```cpp
struct Config {
    skl_fixed_vector<u32, 10> items;  // Max 10 items
};

root.array_numeric<u32>("items", &Config::items)
    .truncate_on_overflow(true);  // Truncate if > 10 items
```

---

## Error Handling

### Comprehensive Error Messages

The library provides detailed error messages with full field paths:

```cpp
[ERROR] Field "__root__:servers:0:port" value(80) must be >= 1024!
[ERROR] Field "__root__:database:host" has invalid value! Min length[1]
[ERROR] Array field "__root__:servers" elements count must be in [min=1, max=10]!
[ERROR] Enum field "__root__:log_level" has invalid value("TRACE")!
        Allowed values:
                Debug
                Info
                Warning
                Error
```

### Field Path Format

Error messages include full hierarchical paths:
- `__root__` - Root JSON object
- `__root__:field_name` - Top-level field
- `__root__:object:field` - Nested object field
- `__root__:array:0:field` - Array element field (with index)

### Exception Safety

The library guarantees:
1. Target object is NEVER modified on validation failure
2. All-or-nothing semantics
3. Strong exception safety guarantee

```cpp
MyConfig config{};  // Original state

try {
    loader.load_validate_and_submit("config.json", config);
} catch (...) {
    // config is unchanged - still in original state
}
```

---

## Best Practices

### 1. Static Configuration Loader Pattern

Use static ConfigNode with lazy initialization:

```cpp
ConfigNode<MyConfig>& get_config_loader() {
    static ConfigNode<MyConfig> loader;
    static bool initialized = false;

    if (initialized) return loader;

    // Configure fields...

    initialized = true;
    return loader;
}
```

### 2. Separate Configuration Module

Encapsulate all config logic in a dedicated module:

```cpp
// config_loader.hpp
namespace app::config {
    ConfigNode<AppConfig>& get_loader();
}

// config_loader.cpp
namespace app::config {
    ConfigNode<AppConfig>& get_loader() {
        // Implementation
    }
}

// main.cpp
#include "config_loader.hpp"

int main() {
    AppConfig config{};
    app::config::get_loader().load_validate_and_submit("config.json", config);
}
```

### 3. Validate Default Values

Always ensure default values satisfy constraints:

```cpp
// GOOD
root.numeric<u16>("port", &Config::port)
    .default_value(8080)
    .min(1024);  // 8080 >= 1024 ✓

// BAD - Will fail at runtime
root.numeric<u16>("port", &Config::port)
    .default_value(80)   // 80 < 1024 ✗
    .min(1024);
```

### 4. Use Type-Safe Enums

Prefer scoped enums (enum class) for better type safety:

```cpp
// GOOD
enum class LogLevel { Debug, Info, Warning, Error };

// ACCEPTABLE
enum Status { Active = 0, Inactive = 1 };
```

### 5. Thread Safety

**Important**: ConfigNode instances are NOT thread-safe.

- Don't share ConfigNode across threads during load/validate/submit
- Static loaders are thread-unsafe
- For multi-threaded apps, use thread_local or instance-per-thread pattern

```cpp
// Option 1: Thread-local
thread_local ConfigNode<MyConfig>& get_thread_local_loader() {
    static ConfigNode<MyConfig> loader;
    // Configure...
    return loader;
}

// Option 2: Instance per thread
void worker_thread() {
    ConfigNode<MyConfig> loader;
    // Configure...
    loader.load_validate_and_submit("config.json", my_config);
}
```

### 6. Error Logging Integration

Integrate with your logging system using the SERROR macros:

```cpp
// The library uses SERROR, SERROR_LOCAL, SERROR_LOCAL_T macros
// Define these to integrate with your logging system
#define SERROR(fmt, ...) my_logger::error(fmt, ##__VA_ARGS__)
```

### 7. Hierarchical Defaults

Set sensible defaults at multiple levels:

```cpp
// Parent with defaults
ConfigNode<Parent> parent;
parent.numeric<u32>("timeout", &Parent::timeout).default_value(30);

// Child inherits sensible defaults
ConfigNode<Child> child;
child.numeric<u32>("timeout", &Child::timeout).default_value(30);
```

### 8. Validation Testing

Test your configuration validation:

```cpp
void test_config_validation() {
    ConfigNode<MyConfig> loader = get_config_loader();

    // Test valid config
    MyConfig config1{};
    loader.load_validate_and_submit("valid_config.json", config1);
    assert(config1.port > 1024);

    // Test invalid config
    MyConfig config2{};
    try {
        loader.load_validate_and_submit("invalid_config.json", config2);
        assert(false && "Should have thrown");
    } catch (const std::exception&) {
        // Expected
    }
}
```

---

## Performance Considerations

### Configuration Loading Performance
- One-time cost during application startup
- JSON parsing is the primary bottleneck
- Validation is fast (single-pass)
- Consider caching loaded configurations

### Memory Usage
- ConfigNode instances should be static (shared)
- Deep copies of ConfigNode can be expensive
- Target POD objects have minimal overhead

### Compilation Time
- Heavy template usage may increase compile times
- Use forward declarations where possible
- Consider compilation units per configuration module

---

## Platform Requirements

- **Compiler**: Clang (latest) with C++23 support
- **Platform**: Linux x86_64
- **Dependencies**: nlohmann::json, magic_enum
- **Features**: Compiler intrinsics are used for optimization

---

## Summary

Skylake Config provides a powerful, type-safe, and user-friendly API for configuration management:

✅ Complete type safety via C++23 concepts
✅ Fluent declarative API
✅ Comprehensive validation
✅ Clear error messages
✅ Zero coupling to application code
✅ Fail-fast semantics
✅ Production-ready design

For more examples, see the `workbench/` directory in the repository.
