# Chat Application

[Engineering notebook](https://docs.google.com/document/d/1hcSdIypMp6nQPjrfDupTkPNUOKl4JdVrWQD7tTFnyR8/edit?usp=sharing)

This project is an application comprised of a C++ server and a Python client. This service allows for real time messaging with multiple clients that are handled by one server.

## Overview

- **Server (C++):**  
  Handles client connections, user registration, authentication, chatroom management, and more.

- **Client (Python):**  
  Provides a simple interface to connect to the server and exchange messages.

## Getting Started

When getting started with our repository, it is first important to ensure you have all of the necessary packages and enviornments set up.

- Linux environment
- C++23-compatible compiler (e.g., g++ (>=14) or clang (>=16) with C++23 support)
- Make
- vcpkg
- CMake (>=3.10)
- pkg-config (for argon2)
- uv-astral (python)

## File Structure

```plaintext
chat/
├── client
│   ├── main.py
│   ├── json_main.py
│   ├── alice.py
│   ├── uv.lock
│   ├── pyproject.toml
│   ├── Makefile
│   ├── main.qml
│   ├── json_main.qml
│   ├── backend.py
│   ├── json_backend.py
│   └── protocols
│       ├── dyde
│       │   ├── client.py
│       │   ├── client_test.py
│       │   └── utils.py
│       └── json
│           ├── client.py
│           └── deserialize.py
└── server
    ├── src
    │   ├── main.cpp
    │   ├── main_test.cpp
    │   ├── connections.cpp
    │   ├── connections.hpp
    │   ├── database.cpp
    │   ├── database.hpp
    │   ├── models.cpp
    │   ├── models.hpp
    │   └── protocols
    │       ├── dyde
    │       │   ├── utils.cpp
    │       │   ├── server.cpp
    │       │   └── ...
    │       └── json
    │           |── json_pipeline.cpp
    │           └── json_pipeline.hpp
    ├── CMakeLists.txt
    ├── CMakePresets.json
    ├── Makefile
    ├── main.sql
    └── README.md
```

## Client

For concision, the direction to set up the server are in a dedicated readme. [ClientREADME](./client/README.md).

### Server

For concision, the direction to set up the server are in a dedicated readme. [Server README](./server/README.md).

## Testing

### Server Testing

- **Running Server Tests:**
  1. Ensure you are in the server directory:
     ```sh
     cd /home/dylangreenx/cs2620/chat/server
     ```
  2. Run the tests using Make:
     ```sh
     make test
     ```
     The tests will build the server (if needed) and run the tests executable.

### Client Testing

- **Running Client Tests:**
  1. Ensure you are in the client directory:
     ```sh
     cd /home/dylangreenx/cs2620/chat/client
     ```
  2. Run the tests using Make:
     ```sh
     make test
     ```
     This will run the unit tests with Python’s unittest module, discovering any test files that match the pattern `*_test.py`.

## Features

- **User Management:**  
  Registration, authentication, and messaging.
- **Chatrooms:**  
  Dynamic creation and management of chatrooms.
- **Concurrency:**  
  Handling multiple client connections simultaneously (via threads in the server).

## Future Enhancements

- Our software is extensible, providing a consistent and repetitive structure to allow for future additions
- To make this a robust platform, we intend to add regression and integration testing to ensure all components function correctly as the software package develops.
- Right now, the wire protocol is not secure and can be intercepted by malicious actors. We will eventually increase the robustness of our service.
