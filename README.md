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
./raft --id <server_id> <ip_address>:<listen_port>:<destination_port>
```

The first option `--id <server_id>` gives the server the specified name or
"server id". This is a friendly name that the server uses to identify itself to
other servers in the cluster, as well as to name thee persistent storage file.

The second option `<ip_address>:<listen_port>:<destination_port>` tells the
server that it should connect to the peer with the given *ip_address* and
*destination_port*, and it should expect to receive communication from this peer
on the given *listen_port*.

🌟 That's it. Now you know all the command line options you need to start a
proper Raft cluster. We'll do that in the next section.

### Start a three server Raft cluster

To start a real Raft cluster, you need to run three copies of the program in
parallel and ensure that the "peer information" you provide to each server in
the cluster matches the information provided to all the other servers.

Here are some commands you can copy-paste into three separate terminals:

```bash
./raft --id alice 127.0.0.1:4000:4010 127.0.0.1:4001:4020
```

```bash
./raft --id bob 127.0.0.1:4010:4000 127.0.0.1:4011:4021
```

```bash
./raft --id carol 127.0.0.1:4020:4001 127.0.0.1:4021:4011
```

✨ And just like that, you're running a Raft cluster!

### A few odds and ends...

#### Reset persistent storage

To reset the persistent storage of a server, use the `--reset` boolean argument.

```bash
./raft --id <server_id> --reset
```

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

        ./raft --id <server_id> <ip_address>:<listen_port>:<destination_port>

        Tells this instance of raft to connect to *ip_address* by sending to
        port *destination_port*, and receiving on *listen_port*. *--id* is
        required to specify the server's name, which is used to maintain its
        persistent storage as well as to identify itself to other servers in the
        cluster.

Cluster Example:
    Start a three server Raft cluster.

        ./raft --id alice 127.0.0.1:4000:4010 127.0.0.1:4001:4020
        ./raft --id bob 127.0.0.1:4010:4000 127.0.0.1:4011:4021
        ./raft --id carol 127.0.0.1:4020:4001 127.0.0.1:4021:4011

Usage:
    --debug  Show debug logs               [bool]
    --help   Print help message            [bool]
    --id     Server identifier             [string]
    --quiet  Show only warnings and errors [bool]
    --reset  Delete server storage         [bool]
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
