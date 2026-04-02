# Agent Guidelines for Point and Figure Project

## Build Commands

### Main Application

```bash
# Debug build
make -f makefile_collect CFG=Debug

# Release build
make -f makefile_collect CFG=Release

# Clean build artifacts
make -f makefile_collect clean
```

### Compiler and Standards

- **Compiler**: GCC 15+ with `-std=c++26`
- **Libraries**: Boost 1.90, spdlog, fmt, range-v3, Howard Hinnant's date, jsoncpp, libdecnumber/mpdecimal, ChartDirector, pqxx (PostgreSQL)
- **Linker flags**: Use `-Wl,-rpath` for runtime library paths
- Include directories: ./src ../common_utilities/include/

## Testing

### Running Tests

```bash
# Build and run decimal_test
make -f decimal_test.mk

# Debug test binaries are in Debug_test/ directory
```

### Test Structure

- Tests follow the `CMyApp` pattern with `Do_StartUp()`, `Do_Run()`, `Do_CheckArgs()` lifecycle
- Use try-catch blocks around application code to capture exceptions
- Tests typically return exit codes: 0=success, 1-5=various error states

## Linting and Code Quality

### Python

```bash
# Lint Python files
ruff check .

# Check for issues (E=errors, F=pyflakes, B=bugbear)
ruff check --select=E,F,B
```

### SQL

```bash
# Lint SQL files
sqlfluff lint .

# SQL dialect: postgres
# Max line length: 120
# Keywords: UPPER, Identifiers: lower, Types: UPPER
```

## Code Style Guidelines

### File Headers

All source files require standard header block:

```cpp
// =====================================================================================
//
//       Filename:  FileName.cpp
//
//    Description:  Brief description
//
//        Version:  X.X
//        Created:  MM/DD/YYYY HH:MM AM/PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:
//
// =====================================================================================
```

Followed by GPL v3 license notice in comment form.

### Naming Conventions

#### Classes

- PascalCase: `PF_CollectDataApp`, `PF_Chart`, `PointAndFigureDB`
- Prefix with domain: `PF_` for Point-and-Figure related classes

#### Functions and Methods

- PascalCase for member functions: `Startup()`, `Run()`, `Shutdown()`
- Use `[[nodiscard]]` on functions whose return values should not be ignored
- Private methods with `_` suffix: `PF_streamer_`, `charts_`

#### Variables

- Member variables: camelCase with trailing underscore: `charts_`, `logger_`
- Static members: static with trailing underscore: `had_signal_`
- Constants: `kMaxBoxes`, `kMinExponent`
- Local variables: camelCase: `new_data`, `streamed_prices`

#### Enums

- PascalCase enum types: `BoxType`, `BoxScale`
- Lowercase enum values with prefix: `e_Integral`, `e_Linear`, `e_Percent`

#### Files

- C++ source: `FileName.cpp`, `FileName.h`
- Python: `FileName.py` (PascalCase)

### Formatting

#### Indentation

- 4 spaces for indentation (no tabs)
- Method descriptions follow pattern:
  ```cpp
  //--------------------------------------------------------------------------------------
  //       Class:  ClassName
  //      Method:  MethodName
  // Description:  brief description
  //--------------------------------------------------------------------------------------
  ```

#### Braces and Spacing

- Allman/BSD style (brace on separate line):
  ```cpp
  bool PF_CollectDataApp::Startup()
  {
      // code
  }
  ```
- Space after type in declarations: `std::string name`
- No space before parentheses in function calls: `function(args)`
- Space after commas: `func(a, b, c)`

#### Line Length

- Maximum 120 characters (Python and SQL)
- C++ uses wrapping for long lines

### Imports and Includes

#### C++ Include Order

1. Standard library headers (alphabetical)
2. Third-party headers (alphabetical)
3. Project headers (relative paths)

```cpp
#include <algorithm>
#include <chrono>
#include <format>
#include <string>
#include <spdlog/spdlog.h>
#include "PF_CollectDataApp.h"
#include "utilities.h"
```

#### Python Imports

```python
import pandas as pd
import matplotlib
import numpy as np
import mplfinance as mpf
```

- Standard library first
- Third-party libraries second
- Use `as` aliases for common libraries

### Types and Templates

#### Type Aliases

```cpp
using PF_Charts = std::vector<std::pair<std::string, PF_Chart>>;
using Box = decimal::Decimal;
using BoxList = std::deque<Box>;
```

#### Modern C++ Features

- Use `std::optional` for optional return values
- Use `std::unique_ptr` and `std::shared_ptr` for ownership
- Use `std::ranges` and views when appropriate
- Use `std::format` instead of printf-style formatting
- Use `std::filesystem` (alias `fs` in namespace)
- Prefer `std::chrono` for time operations

#### Decimal Precision

- Use `decimal::Decimal` for financial calculations
- Set context at startup:
  ```cpp
  decimal::context_template = decimal::IEEEContext(decimal::DECIMAL64);
  decimal::context_template.round(decimal::ROUND_HALF_UP);
  ```

### Error Handling

#### Exception Handling

```cpp
try
{
    // application code
}
catch (const std::system_error& e)
{
    std::cerr << "Category: " << e.code().category().name() << '\n';
    return 3;
}
catch (const std::exception& e)
{
    spdlog::error(std::format("Problem: {}", e.what()));
    return 4;
}
catch (...)
{
    spdlog::error("Unknown problem");
    return 5;
}
```

#### Logging

- Use spdlog with named loggers: `spdlog::info()`, `spdlog::error()`
- Configure logging early in Startup()
- Set log level based on command-line options
- Use file sinks for persistent logging

#### Assertions

- Use `BOOST_ASSERT_MSG()` for precondition checks
- Use `[[nodiscard]]` for critical return values

### Memory Management

#### Ownership

- Use `std::unique_ptr` for exclusive ownership
- Use `std::shared_ptr` for shared ownership (e.g., spdlog loggers)
- Default constructors, copy/move operations should be explicit:
  ```cpp
  PF_CollectDataApp() = delete;
  PF_CollectDataApp(const PF_CollectDataApp &rhs) = delete;
  PF_CollectDataApp(PF_CollectDataApp &&rhs) = default;
  ```

#### Smart Pointers

```cpp
std::shared_ptr<spdlog::logger> logger_;
std::unique_ptr<RemoteDataSource> PF_streamer_;
```

### Database Operations

#### PostgreSQL

- Use libpqxx for database access
- Always use parameterized queries to prevent SQL injection
- Handle transaction boundaries explicitly
- Close connections promptly

```cpp
pqxx::connection c{std::format("dbname={} user={}", db_name, user_name)};
pqxx::work tx{c};
```

### Command Line Interface

- Use CLI11 library for argument parsing
- Provide comprehensive help messages
- Support both short (`-h`) and long (`--help`) options
- Validate arguments in `CheckArgs()` method

### Comments

#### Documentation Style

- Use block comments for class/method descriptions
- Include brief description of purpose
- Mark lifecycle methods clearly

#### Inline Comments

- Keep comments concise and on same line when possible
- Explain why, not what
- Use `//` for implementation details

### SQL Style

- Keywords in UPPER CASE
- Identifiers in lowercase
- Type names in UPPER CASE
- Max line length 120

### Python Style

- Function names: snake_case
- Class names: PascalCase
- Constants: UPPER_CASE
- Use type hints where appropriate
- Maximum line length: 120 characters

### Thread Safety

- Use `std::mutex` for shared state protection
- Consider thread pool for concurrent symbol processing
- Signal handling should set flags, not directly modify state

### Performance

- Use `-march=native -mtune=native` for native CPU optimization
- Enable LTO in Release builds
- Disable stdio synchronization for performance:
  ```cpp
  std::ios_base::sync_with_stdio(false);
  ```
