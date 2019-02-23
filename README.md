<h1 align="center">
  <br>
  <img src="https://raft.github.io/logo/annie-solo.png" alt="Raft logo" width="200">
  <br>
  Raft - An understandable consensus algorithm
  <br>
  <br>
</h1>

## What is Raft?

Raft is a consensus algorithm that is designed to be easy to understand. It's
equivalent to Paxos in fault-tolerance and performance. The difference is that
it's decomposed into relatively independent subproblems, and it cleanly
addresses all major pieces needed for practical systems. We hope Raft will make
consensus available to a wider audience, and that this wider audience will be
able to develop a variety of higher quality consensus-based systems than are
available today.

## Quick Start

```bash
$ make install-deps # Quick install system dependencies (macOS only)
$ make
$ ./raft --help
```

🥳 Congrats, you just got the Raft binary running! Now, let's start a basic Raft server.

### Starting the Raft server

The basic command line format is as follows:

```bash
make
./raft --id <server_id>
```

The first option `--id <server_id>` gives the server the specified name or
"server id". This is a friendly name that the server uses to identify itself to
other servers in the cluster, as well as to name thee persistent storage file.

This command looks for a config file at `./config` and reads in information
about the Raft cluster from there. Here's what a simple config file for a single
server cluster looks like:

```
127.0.0.1 4000
```

This defines a server that listens for client requests on `127.0.0.1` and port
`4000`.

Now start the server like this:

```bash
./raft --id 0 --reset
```

The `--reset` option must be included the first time a server is run to
establish the persistent storage and log files on disk. Read more about
`--reset` and the other command line options below.

🌟 That's it. Now that you know how to start a single Raft server, you're ready
to start an entire Raft cluster. We'll do that in the next section.

### Start a three server Raft cluster

To start a real Raft cluster, you need to run three copies of the program in
parallel and ensure that each command is given a unique server id.

Here is a sample config file for a three server cluster:

```
127.0.0.1 4000 4001 4002
127.0.0.1 5000 5001 5002
127.0.0.1 6000 6001 6002
```

Each line represents a separate server. Since there are three servers in the
cluster, each server needs to open two additional ports to receive connections
from the other servers in the cluster.

Here are some commands you can copy-paste into three separate terminals to start
up the cluster:

```bash
./raft --id 0 --reset
```

```bash
./raft --id 1 --reset
```

```bash
./raft --id 2 --reset
```

✨ And just like that, you're running a Raft cluster!

### Sending commands to the cluster

The purpose of Raft is to make a bunch of servers work together to reliably
update a state machine. So, we'll now discuss how to actually update the state
in the state machine.

**Note:** In this implementation of Raft, we include a sample state machine that
takes terminal commands and runs them in `bash`, returning the output as a
string. The bash state machine conforms to the `StateMachine` interface defined
in `state-machine.h` and you can use it as an example when writing your own
state machine implementation, if you so desire.

First, start the `./client` program which creates a REPL that sends commands to
the Raft cluster. The client automatically handles finding the leader server,
retrying the request with a different server if the leader becomes unavailable,
and displaying the output from running each command.

Here's what a sample run of the client looks like:

```bash
$ ./client
> echo hello
hello
> touch myfile.txt
> ls
myfile.txt
> quit
```

### A few odds and ends...

#### Reset persistent storage

To reset the persistent storage of a server, use the `--reset` boolean argument.
This option is required the first time you start the server.

```bash
./raft --id <server_id> --reset
```

**Note:** Do not use `--reset` when a server is rejoining the cluster after a
crash or shutdown as this will wipe away the persistent state and log which will
cause consistency issues. Adding/removing servers from the cluster as described
in the Raft paper is currently not supported in this implementation.

#### Use a custom configuration file location

```bash
./raft --id <server_id> --config ./my_cool_config_file
```

To use a custom configuration file location, use the `--config` string argument.

#### Show debug logs

To show extremely verbose debug logs, use the `--debug` boolean argument.

```bash
./raft --id <server_id> <peer_info> --debug
```

#### Quiet mode

To show only warnings and errors and hide almost every other log message, use
the `--quiet` boolean argument.

```bash
./raft --id <server_id> <peer_info> --quiet
```

### Command Line Help

Get help from the command line by using the `--help` boolean argument:

```
$ ./raft --help
Raft - An understandable consensus algorithm

Usage:
    ./raft [options] [peers ...]

Minimal Example:
    Start a server that connects to one other server.

        ./raft --id <server_id> --reset

Cluster Example:
    Start a three server Raft cluster.

    Use the config file:
        127.0.0.1 4000 4001 4002
        127.0.0.1 5000 5001 5002
        127.0.0.1 6000 6001 6002

    Run:
    ./raft --id 0 --reset
    ./raft --id 1 --reset
    ./raft --id 2 --reset

Usage:
    --config  Path to configuration file (default = ./config) [string]
    --debug   Show all logs                                   [bool]
    --help    Print help message                              [bool]
    --id      Server identifier                               [int]
    --quiet   Show only errors                                [bool]
    --reset   Delete server storage                           [bool]
```

## Install Dependencies

### Quick Install (macOS only)

macOS users with the [Homebrew](https://brew.sh/) package manager installed can
install all system dependencies in one step.

```bash
make install-deps
```

### Protocol Buffers

Protocol buffers are a language-neutral, platform-neutral, extensible way of
serializing structured data for use in communications protocols and data storage.

See the official
[Protocol Buffers](https://developers.google.com/protocol-buffers/) website to
learn more.

### pkg-config

`pkg-config` is a helper tool used when compiling applications and libraries. It
helps you insert the correct compiler options on the command line.

See the official
[`pkg-config`](https://www.freedesktop.org/wiki/Software/pkg-config/) website
to learn more.

## Additional Raft Resources

- [In Search of an Understandable Consensus Algorithm](https://raft.github.io/raft.pdf)
- [Raft Visualization](https://raft.github.io/)

# License

Copyright (c) Feross Aboukhadijeh and Jake McKinnon
