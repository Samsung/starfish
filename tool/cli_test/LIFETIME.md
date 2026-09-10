# CLI session lifetime

The CLI uses three processes.

| Process | Lifetime |
| --- | --- |
| client | One command |
| daemon | One session |
| engine | Until its daemon exits |

The client and daemon are the same `<TARGETNAME>-cli` binary. The daemon runs
with `--daemon` and starts the sibling `<TARGETNAME>` engine.

```text
time ->

client   [open]       [snapshot]       [close]     one process per command,
                                                   gone when it prints

daemon       [==============================]      one per session, started
                                                   by the open that found none
engine         [============================]      one per daemon
```

A command that arrives with no daemon running starts one. Every command after
that reaches the same daemon, which is why `snapshot` sees the page `open`
loaded.

## Session startup

The session files are:

```text
$HOME/lightweight-web-engine/cli/default.sock
$HOME/lightweight-web-engine/cli/start.lock
```

Only `open` can start a session. Other commands fail when the socket does not
answer.

1. The client tries the socket.
2. If it fails, the client locks `start.lock` and tries again. This prevents
   concurrent `open` commands from starting separate daemons.
3. The client starts the daemon and waits for one startup message.
4. The daemon starts the engine, connects through CDP, and sets up the
   session.
5. The daemon creates the socket and reports `READY`. On failure, it reports
   `ERROR <reason>` instead.

The daemon has 8 seconds to connect to the engine. The client waits 12 seconds
for the startup message. The client timeout must remain longer so the daemon
can report a useful error before the client stops it. These values are defined
in [`Constants.h`](../../src/launcher/cli/Constants.h).

```text
        0s                       8s            12s
daemon  |==== retry the CDP connect ==|              gives up, reports why
client  |==== wait for one line ====================|  still reading, so the
                                                       reason reaches the user
```

Swap the two and the client kills the daemon while it is still retrying. The
user then sees only `no active session`, which is what used to happen.

The socket is created only after CDP setup succeeds. A failed startup therefore
does not leave a usable-looking session behind.

`HOME` may be of any length. `bind` and `connect` carry the socket path in a
108 byte field, and `HOME` alone can be longer than that, so both sides change
directory to the session directory and pass only the file name. See
[`SocketPath.h`](../../src/launcher/cli/SocketPath.h). Passing the whole path
again would make the CLI unusable under a long `HOME`.

## Session shutdown

| Event | Result |
| --- | --- |
| `close` | The daemon removes the socket, stops the engine, and exits |
| `SIGTERM` or `SIGINT` | The daemon cleans up and exits |
| daemon exit | The engine receives `SIGTERM` through `PR_SET_PDEATHSIG` |
| startup failure | The daemon stops the engine and reports the reason |
| startup timeout | The client stops and reaps the daemon it started |

The daemon has no idle timeout. Tests call `close` during teardown and
terminate any process left under the test's temporary `HOME`.

See [`ClientMain.cpp`](../../src/launcher/cli/ClientMain.cpp) and
[`DaemonMain.cpp`](../../src/launcher/cli/DaemonMain.cpp) for the
implementation.
