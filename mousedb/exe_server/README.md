# exe_server

This is the server that provides a frontend to the database. The focus of this project is only on features related to the server such as networking, authentication, and connections.

## Usage

macOS and Windows are not supported.

### Linux

Using Nix is the only supported method.

```sh
nix build
```

## Development

Using Docker is the only supported method.

To start:

```sh
docker compose up --build -d
docker compose attach exe_server
```

To stop:

```sh
docker compose down
```
