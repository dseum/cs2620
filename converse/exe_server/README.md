# exe_server

This is the project for the Converse server executable.

## Usage

```sh
build/exe server --name 0
build/exe serve --name 1 --port 50052 --join 0.0.0.0:50051
build/exe serve --name 2 --port 50053 --join 0.0.0.0:50051
```

## Strategy

We use streams of transactions from the leader to replicas. To maintain atomic operation, we take advantage of SQLite’s implicit row increment without the stronger auto increment constraint and avoid any deletes to keep operations consistent and ordered.

The leader stores the most recent copy and continuously aggregates where replicas received after an identity RPC. This means this is decentralized in a way that not all replicas know each other: that's okay because the leader is selected from the order in which the servers were added. The invariant there is at least one known server that is shared and is in the prefix of the known servers for a node.

For adding new nodes while online, we do serializing and deserializing sqlite database to send a copy and then it's a normal replica. Note that clients also receive this new server in its list through a stream.
