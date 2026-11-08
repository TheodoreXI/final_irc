*This project has been created as part of the 42 curriculum by anel-men, aakroud, ayoakouh.*

# ft_irc

## Description

**ft_irc** is a from-scratch implementation of an IRC (Internet Relay Chat) server in C++98, built without relying on any external networking or IRC libraries. The goal of the project is to understand how a real-time, multi-client network application works under the hood: non-blocking I/O, connection multiplexing, and the design of a text-based protocol that many people still use today.

The server accepts multiple simultaneous client connections (tested with real IRC clients as well as raw tools like `nc`), authenticates them, and lets them communicate through channels using a subset of the standard IRC protocol.

The implementation follows these references:

- [RFC 1459 — Internet Relay Chat Protocol](https://tools.ietf.org/html/rfc1459)
- [RFC 2812 — Internet Relay Chat: Client Protocol](https://tools.ietf.org/html/rfc2812)
- [Modern IRC Client Protocol (ircdocs.horse)](https://modern.ircdocs.horse/)

Implemented features include:

- Client registration (`PASS`, `NICK`, `USER`)
- Private messaging between users (`PRIVMSG`)
- Channel management (`JOIN`, `TOPIC`, `KICK`, `INVITE`)
- Channel modes (`MODE`): invite-only (`i`), topic restriction (`t`), channel key (`k`), operator privilege (`o`), user limit (`l`)
- Operator privileges and permission checks
- Graceful handling of client disconnects (cleanup of channel membership, operator status, and pending invites)
- A single-threaded event loop built around `epoll`, with all client sockets running in non-blocking mode

The project emphasizes correctness of protocol behavior (proper numeric replies, RFC-compliant error handling) as much as it does the underlying network programming.

## Instructions

### Requirements

- A Unix-like environment (Linux; developed and tested on Ubuntu)
- `c++` compiler with C++98 support
- `make`

### Compilation

```bash
make
```

This produces an executable named `ircserv` (or `a.out`, depending on the Makefile target) at the root of the project. Other available Makefile targets:

```bash
make clean     # remove object files
make fclean    # remove object files and the binary
make re        # fclean + make
```

### Running the server

```bash
./ircserv <port> <password>
```

- `<port>` — the TCP port the server will listen on (e.g. `6667`, the standard IRC port)
- `<password>` — the connection password clients must supply via `PASS` before registering

Example:

```bash
./ircserv 4444 mypassword
```

### Connecting to the server

The reference client used for testing this server is **[irssi](https://irssi.org/)**. You can also connect using any other standard IRC client (HexChat, WeeChat, etc.), or manually test the protocol with `nc`.

**With irssi:**

```bash
irssi -c localhost -p 4444 -w mypassword -n alice
```

Or from within irssi:

```
/connect localhost 4444 mypassword
/nick alice
/join #general
```

**With `nc` (for raw protocol testing):**

```bash
nc localhost 4444
PASS mypassword
NICK alice
USER alice 0 * :alice
JOIN #general
PRIVMSG #general :hello everyone
```

## Resources

### Documentation & references

**Protocol standards followed for this implementation:**

- [Modern IRC Client Protocol (ircdocs.horse)](https://modern.ircdocs.horse/) — primary reference for numeric replies, command semantics, and general server behavior
- [RFC 1459 — Internet Relay Chat Protocol](https://tools.ietf.org/html/rfc1459) — original IRC protocol specification
- [RFC 2812 — Internet Relay Chat: Client Protocol](https://tools.ietf.org/html/rfc2812) — updated client protocol specification

**Additional references used for the networking implementation:**

- `man` pages: `socket(2)`, `bind(2)`, `listen(2)`, `accept(2)`, `epoll(7)`, `epoll_ctl(2)`, `epoll_wait(2)`, `fcntl(2)`, `send(2)`, `recv(2)`

### AI usage

AI assistance (Claude) was used during this project in the following ways:

- **Debugging support** — identifying and explaining logic bugs in command handlers (e.g. `PRIVMSG`, `INVITE`, `MODE`, `KICK` failing to check client registration status before treating a nickname as a valid target; client disconnects not being propagated to channel member/operator/invite lists).
- **Test design** — designing manual test scenarios (using `nc`) to reproduce bugs before a fix and confirm expected behavior after a fix, covering both the buggy edge cases and standard regression paths.
- **Concept explanations** — clarifying specific C++98 idioms and networking concepts used in the codebase, such as the safe erase-while-iterating pattern for `std::map` (`container.erase(it++)`) and non-blocking socket/epoll behavior.
