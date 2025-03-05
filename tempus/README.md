# tempus

This project simulates a distributed system using multiple processes (referred to as “nodes” or “virtual machines (VMs)”) that exchange messages and maintain logical clocks. Each node:

Runs at a random “clock rate” (1 - 6 events per second).
Performs random “internal” events or sends messages to other nodes.
Receives messages asynchronously and updates its logical clock accordingly.
Logs are recorded per node (e.g., rank_0.log, rank_1.log, rank_2.log).

## Requirements

- clang (>= 19)
- clang-tools (>= 19)
- CMake (>= 3.30)
- Ninja
- vcpkg (manifest mode)
- (Adjust version requirements to match your local environment if they differ.)

## Building and Running

Configure the project (choose either debug or release):

1. Configure the project (choose either debug or release):

```bash
cmake --workflow --preset debug
```

or

```bash
cmake --workflow --preset release
Run the compiled executable:
```

2. Run the compiled executable:

```bash
build/exe
```

By default, the code creates three processes (ranks 0, 1, and 2) and runs them all concurrently (via fork on Unix-like systems).

3. Check logs: The simulation runs for a fixed duration (currently set to 60 seconds). Each process writes its log to a file named rank\_<node number>.log, for example rank_0.log. Look at these logs to see the event ordering, message sends/receives, internal events, and logical clock updates.

## Testing

```sh
build/exe_test
```

## Project Structure

```css
.
├─ main.cpp
├─ logic.hpp
├─ logic.cpp
├─ logger.hpp
├─ logger.cpp
└─ README.md
```

### main.cpp

- Primary entry point for the simulation.
- Spawns multiple child processes (one per VM). Each process is assigned a rank (0, 1, 2, etc.) and a unique port.
- Establishes socket connections between the processes so they can send messages to one another.
- Calls `runNode(...)` (defined in logic.cpp) on each process to start the simulation loop.

### logic.hpp / logic.cpp

- Defines the Message struct and the core function `runNode(int rank, Logger &logger, const std::vector<int> &channels, int num_vms)` that:
  1. Sets up the node’s logical clock.
  2. Receives incoming messages on separate channels and pushes them into a queue.
  3. Executes random events (internal, send to one or multiple other nodes) on each iteration.
  4. Updates the logical clock on send or receive events as per logical clock rules (i.e., clock = max(local, received) + 1).
  5. Logs everything in real-time.
- Contains the event loop that continues until the simulation time elapses (currently 60 seconds).

### logger.hpp / logger.cpp

- Provides a simple thread-safe file-based logger class.
- Each node constructs a Logger that writes to `rank_<node number>.log`.
- Uses `std::mutex` to ensure logs are written sequentially when multiple threads (e.g. message-receiving thread) access it.

## Simulation Flow

1. Spawning Processes

- main.cpp uses fork() to create num_vms child processes. Each process:
  - Binds and listens on its own port.
  - Accepts or connects to the other processes to form a fully connected set of channels (TCP sockets).

2. Starting `runNode`

- After the sockets are set up, `runNode(...)` is invoked in each child process. This function:
  - In the main thread, it loops at the rate defined by a random “clock rate” for each node (events per second).
  - Chooses a random event in each iteration (internal event or send to others). All events trigger a logical clock increment.
  - When a message arrives, the receiving node updates its logical clock to max(local_clock, message_clock) + 1.

3. Logging

- Each node logs every event to its own `rank_<node number>.log` file (initialized by Logger in logger.cpp).
  - You will see entries for `[SEND]`,`[RECEIVE]`,`[INTERNAL]`,” and so on.

4. End of Simulation

- The loop runs for a fixed duration (60 seconds by default).
- Once time is up, the processes shut down and close the sockets.
- main.cpp waits for all child processes to finish before exiting.

## Customization

- Number of Nodes (num_vms)

  - Currently set to 3 in main.cpp. You can change this but must ensure the connection logic (port usage, indexing) matches the new value. For the purposes of this assignment it does not need to be changed.

- Simulation Time

  - Specified by `DURATION_SECONDS` in logic.cpp. Adjust this to run shorter or longer tests.

- Clock rate

  - Specified by `MAX_CLOCK_RATE` in logic.cpp. By default this is set to 6.

- Random Event Distribution

  - Controlled in logic.cpp, where random values pick internal events versus sending to one or multiple peers. Controlled by `INTERNAL_EVENT_PROBABILITY_CAP` in logic.cpp. By default this is set to 10.

- Port Numbers
  - By default, the code starts at 17050 and increments by 1 for each node. Tweak in main.cpp if needed, making sure the ports are not in use on your system.

