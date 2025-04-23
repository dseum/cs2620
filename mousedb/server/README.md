# server

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
docker compose attach shell
```

`cd` into the directory where you want to work and follow the instructions in the corresponding `README.md`.

To stop, run `docker compose down`. This leaves volumes that are used for caching. To delete them, run `docker compose down --volumes`.
