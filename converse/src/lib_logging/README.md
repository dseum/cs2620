# lib_logging

This is the project for the logging library that can be used with any other project.

## Usage

```cpp
#include <converse/logging/core.hpp>

namespace lg = converse::logging;

int main() {
    lg::init(lg::sink_type::console);
    lg::write(lg::level::info, "Hello, World!");
}
```

After `init`, the sink exists globally and will be used by all subsequent calls to `write`.
