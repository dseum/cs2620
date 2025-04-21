# lib_database

This is the database that the server provides a frontend to. The focus of this project is only on features related to the database such as parsing, querying, and storage.

```cpp
#include <mousedb/database/core.hpp>

using namespace mousedb::database;

int main() {
    Database db("/var/lib/mousedb/data"); // This is the default directory

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

## Usage

macOS and Windows are not supported.

### Linux

Using Nix is the only supported method.

```sh
nix build
```

## Development

Using Docker is the only supported method.

To start, run:

```sh
docker compose up --build -d
docker compose attach lib_database
```

This will attach you into a shell with the correct environment.

- To build, run `cmake --workflow build-<target>`
- To build and test, run `cmake --workflow test-<target>`
- To test, run `ctest --preset <target>`
- To bench, build and run `build/exe/bench`

To stop, run `docker compose down`. This leaves volumes that are used for caching.
