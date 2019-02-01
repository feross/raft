# Raft - An understandable consensus algorithm

## What is Raft?

Raft is a consensus algorithm that is designed to be easy to understand. It's
equivalent to Paxos in fault-tolerance and performance. The difference is that
it's decomposed into relatively independent subproblems, and it cleanly
addresses all major pieces needed for practical systems. We hope Raft will make
consensus available to a wider audience, and that this wider audience will be
able to develop a variety of higher quality consensus-based systems than are
available today.

### Learn more about Raft

- [In Search of an Understandable Consensus Algorithm](https://raft.github.io/raft.pdf)
- [Raft Visualization](https://raft.github.io/)

## Getting Started

### Running a simple raft cluster

```bash
make
./raft <ip_address>:<receive_port>:<destination_port> --id <server name>
```

Tells this instance of raft to connect to ip_address by sending to
port destination_port, and receiving on receive_port.  Id is required to specify
the server's name, which is used to maintain its persistent storage

As a simple example running on localhost (127.0.0.1), you might in three
different terminals:

terminal A
```bash
make
./raft 127.0.0.1:4000:4001 127.0.0.1:8001:8000 --id alice
```
terminal B
```bash
./raft 127.0.0.1:4001:4000 127.0.0.1:6001:6000 --id bob
```
terminal C
```bash
./raft 127.0.0.1:8000:8001 127.0.0.1:6000:6001 --id carol
```

## Dependencies

### Install on macOS

macOS users with the [Homebrew](https://brew.sh/) package manager installed can
install all system dependencies in one step.

```bash
make install-deps
```

### Protocol Buffers

Protocol buffers are a language-neutral, platform-neutral, extensible way of
serializing structured data for use in communications protocols and data storage.

#### Other platforms

See the official [Download Protocol Buffers](https://developers.google.com/protocol-buffers/docs/downloads) page.