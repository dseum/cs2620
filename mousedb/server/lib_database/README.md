# lib_database

This is the database that the server provides a frontend to. The focus of this project is only on features related to the database such as parsing, querying, and storage.

```cpp
#include <mousedb/database/core.hpp>

using namespace mousedb::database;

int main() {
    Database db("/var/lib/mousedb"); // This is the default directory

    db.execute("PUT key value");
    db.execute<Action::PUT>("key", "value"); // This will update the value

    std::optional<std::string> value = db.execute<Action::GET>("key");
    assert(value.value() == "value");

    db.execute<Action::DELETE>("key");
    value  = db.execute<Action::GET>("key");
    assert(!value.has_value());
    return 0;
}
```

### Development

- To build, run `cmake --workflow build-<target>`
- To build and test, run `cmake --workflow test-<target>`
- To test, run `ctest --preset <target>`
- To bench, build and run `build/exe/bench`
