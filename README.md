
# CIRCE (Core Internet Relay Chat Engine)

CIRCE is a versatile and robust library designed to serve as a foundational framework for implementing modern IRC-like chat applications. It encapsulates essential components and functionalities required for a modern chat protocol, offering developers a comprehensive base for building efficient and scalable chat systems.

## Prerequisites

Before building or running CIRCE, ensure you have the following dependencies installed:

### Required:

- **glib2.0:** A fundamental library for C application development. Installation instructions for various systems can be found online, and it is typically included in Linux distributions.
- **jansson:** A C library for encoding, decoding, and manipulating JSON data. [The jansson documentation](https://jansson.readthedocs.io/en/latest/gettingstarted.html).
  You can install it on Ubuntu using the following command:

  ```bash
  sudo apt-get install libjansson-dev
  ```

### Optional:

- **Unity (by ThrowTheSwitch):** This optional library is used for unit testing. You can find more information and installation instructions at the official Unity repository: [Unity GitHub](https://github.com/ThrowTheSwitch/Unity).

## Usage

### Building the Project

To build CIRCE, simply navigate to the project directory and run:

```bash
make
```

### Running the Server and Client

To execute the server, use the following command: *The flags are optional*.

```bash
./bin/server --ip 127.0.0.5 --port 1234
```

To run the client, use:

```bash
./bin/client --ip 127.0.0.5 --port 1234
```

### Chat Commands

Once the client is running, the following chat commands are available:

- `\help` — Displays the list of available commands.
- `\echo` — Echoes the command back to the user.
- `\login` — Sends a login JSON message to the server.
- `\leave` — Closes the connection to the server.
- `\logout` — Logs out from the server.
- `\status <ACTIVE/AWAY/BUSY>` — Changes the user's status.
- `<no command>` — Sends a message to the main chat room.

---

Feel free to contribute, raise issues, or submit pull requests to help improve CIRCE.
```
