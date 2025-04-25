# client

This is the client that provides a remote frontend to the server. The focus of this project is only on connecting to the server.

## Usage

### Linux and macOS

Using Nix is the only supported method.

```sh
nix build
```

### Windows

> WIP

You must install the dependencies:

- [CMake](https://cmake.org/download/)
- [Ninja](https://ninja-build.org/)

```sh
cmake --workflow windows-release
```

## Development

Using Docker is the only supported method.

To start, run:

```sh
docker compose up --build -d
docker compose attach shell
```

To stop, run `docker compose down`. This leaves volumes that are used for caching. To delete them, run `docker compose down --volumes`.
