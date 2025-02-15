# dyde Server Documentation

This README details the dyde protocol design, its primitives, available operations (both client and server), and how to build, run, and test the dyde server.

---

## Overview

The dyde protocol is designed with clarity and efficiency in mind. It defines a set of primitives and operations that allow clients and servers to communicate using a binary format. The protocol is structured into two parts:

- **Client Operations:** For user sign-up, sign-in, messaging, and conversation management.
- **Server Operations:** For server-side message processing and managing unread message counts.

The following sections outline the data types, the available operations, and practical examples of usage.

---

## Design Philosophy

The dyde protocol uses fixed-size binary representations for primitives, ensuring predictable message sizes and efficient serialization/deserialization. The core idea is to use a small set of basic types that can be combined to form complex operations, such as user management and messaging. If you wish to explore more about the data structures and the protocol structure, look further below.

---
## Building and Running the Server

The dyde server is built using C++ and relies on a set of components that handle network connections, request serialization/deserialization, and database operations. Below are the instructions to build, run, and migrate the database.

### Prerequisites

- A C++ compiler that supports C++17 or later.
- Boost libraries (for command-line parsing and testing).
- A working SQL database, with migration SQL statements provided in `main.sql`.
- Installed VCPKG (version 2022-12-27+)
- Installed CMake (version 3.26.0+)
- Installed Make (version 4.3+)
- Installed Boost (version 1.81.0+)

### Usage

To run the dyde or json server, first run the respective make-build command. This compiles the necessary executable for the server to be run.

```bash
make dyde-build
make json-build
```

```bash
make json-test
make dyde-test
```

The executable will be built into the .build folder which leads us to the next key commands.

```bash
./build/exe migrate
```

Migrating will move our database schema from main.sql into our main.db. This is necessary for server operations to be performed.


```bash
.build/exe serve --host 0.0.0.0 --port 54100
```

There are default ports for this command, but optionally can be run with specified commandline arugments as well.

## Data Primitives

The protocol defines the following fundamental types:

- **Bool**: A boolean value stored as a single byte `(uint8_t)`.
- **Uint8**: An 8-bit unsigned integer `(uint8_t)`.
- **Uint64**: A 64-bit unsigned integer `(uint64_t)`.
- **Size**: A 64-bit length or size value `(Uint64)`.
- **Id**: A unique identifier represented as a 64-bit value `(Uint64)`.
- **String**: A variable-length text string stored as a `Size` (denoting length) followed by an array of characters.

### Special Types

- **ServerOperationCode**: A code representing server-side operations `(uint8_t)`.
- **ClientOperationCode**: A code representing client-side operations `(uint8_t)`.
- **StatusCode**: A code indicating the status of an operation `(uint8_t)`.

These types are used consistently across both client and server operations to ensure that messages are correctly encoded and decoded.

---



## API Operations

The API is divided into client and server operations. Each operation consists of a request and a corresponding response. This Diagram layout is length, but feel free to explore futher if you wish.

### Client Operations

1. **SignupUser**
   - **Request:**
     - `username` (String)
     - `password` (String)
   - **Response:**
     - `status_code` (StatusCode)
     - `user_id` (Id)

2. **SigninUser**
   - **Request:**
     - `username` (String)
     - `password` (String)
   - **Response:**
     - `status_code` (StatusCode)
     - `user_id` (Id)

3. **SignoutUser**
   - **Request:**
     - `user_id` (Id)
   - **Response:**
     - `status_code` (StatusCode)

4. **DeleteUser**
   - **Request:**
     - `user_id` (Id)
     - `password` (String)
   - **Response:**
     - `status_code` (StatusCode)

5. **GetOtherUsers**
   - **Request:**
     - `user_id` (Id)
     - `query` (String) – search filter for usernames
     - `limit` (Size) – maximum number of results
     - `offset` (Size) – pagination offset
   - **Response:**
     - `status_code` (StatusCode)
     - `users_size` (Size)
     - `users` – an array of:
       - `user_id` (Id)
       - `username` (String)

6. **CreateConversation**
   - **Request:**
     - `send_user_id` (Id)
     - `recv_user_id` (Id)
   - **Response:**
     - `status_code` (StatusCode)
     - `conversation_id` (Id)

7. **GetConversations**
   - **Request:**
     - `user_id` (Id)
   - **Response:**
     - `status_code` (StatusCode)
     - `conversations_size` (Size)
     - `conversations` – an array of:
       - `conversation_id` (Id)
       - `user_id` (Id)
       - `username` (String)
       - `unread_count` (Size)

8. **DeleteConversation**
   - **Request:**
     - `user_id` (Id)
     - `conversation_id` (Id)
   - **Response:**
     - `status_code` (StatusCode)

9. **SendMessage**
   - **Request:**
     - `user_id` (Id)
     - `conversation_id` (Id)
     - `content` (String)
   - **Response:**
     - `status_code` (StatusCode)
     - `message_id` (Id)

10. **GetMessages**
    - **Request:**
      - `conversation_id` (Id)
      - `last_message_id` (Id) – for pagination or incremental fetching
    - **Response:**
      - `status_code` (StatusCode)
      - `messages_size` (Size)
      - `messages` – an array of:
        - `message_id` (Id)
        - `send_user_id` (Id)
        - `is_read` (Bool)
        - `content` (String)

11. **DeleteMessage**
    - **Request:**
      - `conversation_id` (Id)
      - `message_id` (Id)
    - **Response:**
      - `status_code` (StatusCode)

### Server Operations

1. **SendMessageRequest**
   - **Request:**
     - `conversation_id` (Id)
     - `message_id` (Id)
     - `send_user_id` (Id)
     - `content` (String)
   - **Response:**
     - `status_code` (StatusCode)

2. **UpdateUnreadCountRequest**
   - **Request:**
     - `conversation_id` (Id)
     - `unread_count` (Size)
   - **Response:**
     - `status_code` (StatusCode)

---

