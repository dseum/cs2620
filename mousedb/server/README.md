# Server

A single binary (`exe_server`) that hosts a key-value store, maintains a
**Hybrid-Logical Clock (HLC)** for versioning, and participates in a
multi-leader mesh.  It speaks a simple frame protocol (see `server.hpp`)
over TCP.

---

## Features

| Feature | Notes |
|---------|-------|
| **HLC stamping** | Server advances its own clock for every write; client clocks are ignored. |
| **Multi-leader replication** | Peers discover each other with `IDENTIFY` and `PEER` frames. |
| **CRDT-Based Conflict Resolution** | Relies on Last-Writer-Wins CRDT to ensure eventual consistency accross servers |
---

## Usage

```bash
# From exe_server root
docker compose up --build -d
#or if already built"
docker compose up -d
#then
docker compose attach shell
```

Once in the shell, there are the following directories

```bash
exe_server  flake.lock  flake.nix  lib_database
```
In lib_database, you need to build the release target and install it with cmake --install build. While the build will be cached to your filesystem, you will have to install it any time you down the container since the install is intentionally not persistent.

```bash
cd lib_database
cmake --install build
```

After this has been complete, the server can be built in the exe_server directory.

```bash
cd ../exe_server
cmake --workflow build-<target>
```
The executable will be at build/exe/server_main

When running a multi-server setup, ensure that the first server starts on the lowest port number. To join a server to an existing server, use the --join flag.

``` bash
# Simple Example
# Basic standalone node (listen on :9000)
./build/exe/server_main --port 9000
# Join an existing mesh (this node :9001, join 10.0.0.5:9000)
./build/exe/server_main --port 9001 --join 10.0.0.5:9000
```