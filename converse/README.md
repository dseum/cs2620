# Converse

## Requirements

- clang (>=18)
  - (==19): incompatible due to a linker bug
- clang-tools (>=18)
- CMake (>=3.30)
- Ninja
- vcpkg (manifest mode)
- pkg-config
- autoconf
- automake
- autoconf-archive
- libtool

## Commands

### Build

```sh
cmake --workflow [debug|release]
```

### Execute

```sh
build/src/exe_[client|server]/exe
```

## Projects

| Project         | Description                            |
| --------------- | -------------------------------------- |
| src/exe_client  | [README.md](src/exe_client/README.md)  |
| src/exe_server  | [README.md](src/exe_server/README.md)  |
| src/lib_logging | [README.md](src/lib_logging/README.md) |
| src/lib_proto   | [README.md](src/lib_proto/README.md)   |
