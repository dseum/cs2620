# dyde Client Library Documentation

This README explains the design, usage, and API of the dyde client library. The client library is written in Python and has both dyde and JSON protocol implementations, enabling operations such as user sign-up, sign-in, conversation management, and messaging.

---

## Overview

The dyde client library provides a set of classes to construct, serialize, and process client requests and responses. These classes encapsulate various operations such as signing up, signing in, signing out, deleting a user, searching for other users, managing conversations, and sending or retrieving messages. The library also provides corresponding response classes that handle JSON deserialization.

## Building and running the Client

Similarly to the server, testing can be run with the following command:

```bash
make test
```

This will run the whole test suite and allow for efficient testing. To run the actual application, the user must decide to run either the json or the dyde client. Ensure the server you are running is for the right protocol.

```bash
uv run .\main.py --host 127.0.0.1 --port 54100

```

```bash
uv run .\json_main.py --host 127.0.0.1 --port 54100
```

Running main will give you the dyde protocol, and running json main will give you the Json protocol. Like the server, this takes commandline arguments for the port and the host.

## Data Primitives

The library leverages several basic types defined in the dyde protocol:

- **OperationCode:** Indicates the type of client operation (e.g., sign-up, sign-in).
- **StatusCode:** Used in responses to indicate success or failure.
- **Size:** Represents the length of the JSON payload.
- **Id and String:** Represent user IDs, conversation IDs, and text content.

The primitives are wrapped by helper classes (such as `SizeWrapper`, `StringWrapper`, etc.) that handle converting Python types into a format suitable for network transmission. The messages are wrapped in the appropriate format for the server to recieve them.
