# CLI integration test harness

`run_cli_test.py` tests the LWE CLI against a Starfish build. It discovers
Python test cases, gives each case an isolated session, and runs the public CLI
commands as child processes.

The test cases exercise the public command interface and session lifecycle.
They cover end-to-end workflows, concurrent clients, and failure cleanup.

## Prerequisites

- Linux
- Python 3
- Starfish built with `CLI=1`. This also enables CDP.
- The `test` submodule checked out, because the cases and page fixtures live
  under `test/cli/`.

For example:

```sh
cmake -S . -B out/headless -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBACKEND=glib_headless \
  -DSHELL=glib_headless -DCLI=1
cmake --build out/headless --target starfish.cli.executable
```

The build creates the CLI and the engine as sibling binaries. An `open`
command that finds no session starts a CLI daemon, which is the same CLI
binary run with `--daemon`. That daemon starts the engine next to it and
keeps the session alive. See [STRUCTURE.md](STRUCTURE.md) for the three
processes involved.

## Run

The default binary is
`out/headless/bin/lightweight-web-engine-cli`.

```console
$ python3 tool/cli_test/run_cli_test.py
```

Pass another CLI binary when the build uses a different output directory or
target name.

```sh
python3 tool/cli_test/run_cli_test.py \
  --binary out/release/bin/lightweight-web-engine-cli
```

The runner exits with zero when every discovered test passes. It exits with
one when any test fails.

The harness removes `DISPLAY` from CLI child processes to keep each session
headless.

## Add a test

Add a `test_*.py` file under `test/cli/`. Subclass `CLITestCase` from
`driver.support` so the test gets an isolated `HOME`, a local HTTP server, and
process cleanup. The runner discovers the file automatically, so there is no
test list to update.

Put pages and expected output under `test/cli/fixtures/`. Use
`self.resource_url(<name>)` when a test needs to load a page through HTTP.

## More

- [STRUCTURE.md](STRUCTURE.md): what each file does and what talks to what.
- [LIFETIME.md](LIFETIME.md): the session policy the tests rely on. Who
  starts what, who waits how long, and who dies when.
