# Client

This is a simple test client for the key-value server.

The client connects to a server, then repeatedly sends random
`PUT` (write) and `GET` (read) requests over TCP. It sends 70% writes and 30% reads to simulate standard network traffic.

---

## Usage

```bash
```bash
# From client root
docker compose up --build -d
#or if already built"
docker compose up -d
#then
docker compose attach shell
```
The executable will be at build/exe/client_main.

```bash
#Simple Example
./build/exe/client_main --host 127.0.0.1 --port 9000
```