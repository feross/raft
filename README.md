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

## Dependencies

### Install on macOS

macOS users with the [Homebrew](https://brew.sh/) package manager installed can
install all system dependencies in one step.

```bash
make install-deps
```

### Protocol Buffers

Protocol buffers are a language-neutral, platform-neutral, extensible way of serializing structured data for use in communications protocols and data storage.

#### Other platforms

See the official [Download Protocol Buffers](https://developers.google.com/protocol-buffers/docs/downloads) page.
