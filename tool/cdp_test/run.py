#!/usr/bin/env python3

"""External CDP tests for Starfish.

Two layers, run from one entry point:

    schema    Send every listed CDP command with no parameters and check that
              the response carries the field names the pinned protocol
              declares non-optional. Broad and cheap, but it only checks
              shape: the protocol states no values, so neither can this.

    behavior  Run Chromium's own inspector-protocol tests unmodified and diff
              their output against the -expected.txt that ships with them.
              Narrower, but the expectations come from the people who define
              CDP rather than from us.

No test is written here. Each layer keeps one file, tool/cdp_test/testlist-
<layer>.json, which is both the list of what to run and the result of the
last run:

    {"entries": {"Browser.getVersion": {"expected": "pass"},
                 "Animation.disable":  {"expected": "fail",
                                        "category": "not-implemented",
                                        "detail": "... returned -32601"}}}

An entry expected to pass runs on every run. An entry expected to fail runs
only under --include-fails, or when PATTERN or --list names it.

Every run writes its results back, and only for the entries it ran, so a
subset run updates its part and leaves the rest of the file alone. An entry
that was expected to pass and now fails is written as expected-fail, and the
run also reports it as FAIL and exits non-zero. An entry that was expected to
fail and now passes is promoted, which is why promotion needs --include-fails
or a name.

A category is derived from what the run saw, never from the expected output,
and names the observation rather than a cause: timeout, output-differs,
script-error, not-implemented, bad-params, no-connection, harness-error, and
failed for the schema layer. detail carries the first line that differs.
"""
import argparse
import concurrent.futures
import fnmatch
import pathlib
import sys
import time

SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
# tool/, so that driver.launcher reaches common and repo_paths.
sys.path.insert(0, str(SCRIPT_DIR.parent))

from driver import (behavior, case, commands as command_table, launcher,  # noqa: E402
                    testlist)

# What --workers means when given without a number. Chosen from one full
# behavior sweep: 4 at a time finished it in about a quarter of the time and
# left the time each entry took unchanged, so no entry was starved into a
# timeout. That was one machine, so it says which way to lean, not what any
# run will cost. Higher values were not tried on a full sweep.
DEFAULT_WORKERS = 4
# 16 browsers at once were checked to start, take their own port and answer.
# Beyond that nothing is known, so --workers says so rather than refusing.
VERIFIED_WORKERS = 16
# Enough to see which part of the suite is expensive, short enough to read.
SLOWEST_SHOWN = 5
# Fits the longest label in the timing report, "at the limit", plus a gap.
LABEL_WIDTH = 14
# Worst case past which a run is worth a word about --workers first: every
# entry using its whole timeout. Ten minutes leaves a schema sweep, which is
# short whatever the machine, unasked and catches a behavior sweep.
LONG_RUN_SECONDS = 600
# Everything downloaded lives in the test submodule, next to the other test
# corpora. Only our code and the two lists live here.
SUITE_DIR = launcher.REPOSITORY_DIR / "test" / "cdp"
PROTOCOL_DIR = SUITE_DIR / "protocol"
INSPECTOR_DIR = SUITE_DIR / "inspector-protocol"


def require(path, what):
    if not path.exists():
        raise SystemExit(
            "%s is missing at %s.\n"
            "The CDP test data lives in the 'test' submodule; run\n"
            "  git submodule update --init test" % (what, path))
    return path


class Layer:
    """What differs between the two layers, so the runner can share the rest."""

    def __init__(self, name, list_path, revision, known, run_one, label,
                 total=None):
        self.name = name
        self.list_path = list_path
        self.revision = revision
        self.known = known           # every entry that may appear in the list
        self.run_one = run_one       # (starfish, entry, timeout) -> None
        self.label = label
        # For schema the list covers a subset of the protocol, so the wider
        # count is carried separately.
        self.total = total if total is not None else len(known)


def schema_layer(args):
    protocol_dir = require(PROTOCOL_DIR, "the CDP protocol definition")
    table = command_table.load(protocol_dir)
    # Only the commands this layer can send and judge are listable. The rest
    # are excluded by derivation, not by an entry saying so.
    listable = command_table.reachable(table)
    return Layer(
        name="schema",
        list_path=SCRIPT_DIR / "testlist-schema.json",
        revision=command_table.revision(protocol_dir),
        known={m: c for m, c in table.items() if m in listable},
        total=len(table),
        run_one=lambda starfish, entry, timeout: case.run(
            starfish, table[entry], timeout),
        label=lambda entry: entry)


def behavior_layer(args):
    suite_dir = require(INSPECTOR_DIR, "the inspector-protocol suite")
    known = set(behavior.entries(suite_dir))
    return Layer(
        name="behavior",
        list_path=SCRIPT_DIR / "testlist-behavior.json",
        revision=(suite_dir / "revision.txt").read_text().strip(),
        known=known,
        run_one=lambda starfish, entry, timeout: behavior.run(
            starfish, suite_dir, entry, timeout),
        label=behavior.label)


LAYERS = {"schema": schema_layer, "behavior": behavior_layer}


def counts_for(layer, active, skipped):
    """The counts stored beside the entries, as this run left them.

    total is the wider figure the layer is judged against, which for schema is
    every command in the protocol, not just the ones it can send.
    """
    return {
        "total": layer.total,
        "pass": len(active),
        "fail": len(skipped),
    }


def run_layer(args, layer):
    listed_revision = testlist.revision_of(layer.list_path)
    if listed_revision and listed_revision != layer.revision:
        print("WARNING %s: the list was built against %s but upstream is now "
              "%s; rerun with --include-fails" %
              (layer.name, listed_revision, layer.revision))

    # Validate before running: a stale list should fail now, not after
    # minutes of browser starts. The claim comes first, so a second run says
    # so immediately instead of after its first entry.
    testlist.hold(layer.list_path)
    was_active, was_skipped = testlist.load(layer.list_path, layer.known)

    # A selection picks entries whatever the list expects of them, because a
    # person debugging one test should not have to promote it first.
    if args.pattern:
        to_run = sorted(name for name in layer.known
                        if fnmatch.fnmatch(name, args.pattern))
        if not to_run:
            raise SystemExit("%s: no entry matches %s" %
                             (layer.name, args.pattern))
    elif args.list:
        to_run = sorted(testlist.names(args.list, layer.known))
    else:
        # A plain run covers the entries expected to pass, so it can only
        # demote. Reaching an expected-fail entry needs --include-fails.
        to_run = was_active + (sorted(was_skipped) if args.include_fails
                               else [])
    was_active_set = set(was_active)
    confirm_long_run(args, layer, len(to_run))

    now_active, now_skipped, regressions = [], {}, []
    ran = set()

    def save_progress():
        """Merge what has run so far into the list, rewrite it, and return the
        merged (active, skipped) so the summary can describe the whole file.

        Results go back into the layer's own list, and only for the entries
        that ran: everything else keeps the verdict the file already held, so
        a selection updates its part and leaves the rest as the last full run
        left it.

        Called after every entry rather than once at the end. A sweep costs
        about a second per entry and Ctrl-C kills the runner outright, since
        launcher's handler re-raises the signal with its default disposition,
        so a run that only saved at the end would lose everything it measured.
        """
        active = now_active + [entry for entry in was_active
                               if entry not in ran]
        skipped = dict(now_skipped)
        for entry, record in was_skipped.items():
            if entry not in ran:
                skipped[entry] = record
        testlist.save(layer.list_path, layer.name, layer.revision,
                      sorted(active), skipped,
                      counts_for(layer, active, skipped))
        return active, skipped

    def measure(entry):
        """Run one entry in a worker thread and time it.

        Nothing here touches the list: a worker returns its verdict and the
        thread that owns the list does the recording, so the one-writer rule
        holds however many workers there are.
        """
        started = time.monotonic()
        category, detail = classify(args, layer, entry)
        return entry, category, detail, time.monotonic() - started

    # map hands results back in submission order, so the progress numbers keep
    # meaning what they did when the run was sequential and a parallel run can
    # be diffed against one. It costs nothing in wall time: workers run ahead
    # while an earlier entry is still being waited for.
    started = time.monotonic()
    durations = {}
    pool = concurrent.futures.ThreadPoolExecutor(max_workers=args.workers)
    with pool:
        for position, result in enumerate(pool.map(measure, to_run), 1):
            entry, category, detail, spent = result
            durations[entry] = spent
            name = layer.label(entry)
            if category == "active":
                now_active.append(entry)
                print("%d/%d PASS %s" % (position, len(to_run), name))
            else:
                now_skipped[entry] = {"category": category, "detail": detail}
                # Only a demotion is a failure of this run: an entry already
                # expected to fail, which fails again, is the state the list
                # records.
                if entry in was_active_set:
                    regressions.append(name)
                    print("%d/%d FAIL %s: %s: %s"
                          % (position, len(to_run), name, category, detail))
                else:
                    print("%d/%d %s %s"
                          % (position, len(to_run), category, name))
            ran.add(entry)
            save_progress()
    elapsed = time.monotonic() - started

    # Saved once more so a run over no entries still refreshes the header.
    merged_active, merged_skipped = save_progress()

    # A demotion is counted as failed rather than skipped, so the totals say
    # what this run observed instead of what the rewritten list now holds.
    passed = len(merged_active)
    skipped = len(merged_skipped) - len(regressions)
    print(summary(layer.name, passed, len(regressions), skipped))
    print(timing(layer.name, durations, elapsed, args))
    return (1 if regressions else 0), (passed, len(regressions), skipped)


def confirm_long_run(args, layer, count):
    """Offer a faster way out before a long run starts, and ask to go ahead.

    Only when the worker count was left alone: someone who passed --workers
    has already made this choice. Skipped when the estimate is short, and
    when nothing is there to answer, so CI never waits on a prompt.
    """
    if args.workers_given or not sys.stdin.isatty():
        return
    # How long this takes depends on the machine, so the gate is the work
    # itself: entries times the timeout each may spend, over the workers
    # sharing them. No time is quoted, because a number measured here would
    # not hold anywhere else.
    if count * args.timeout / args.workers < LONG_RUN_SECONDS:
        return
    print("%s: this runs all %d entries, each in a browser of its own, one "
          "after another." % (layer.name, count))
    print("--workers runs %d at a time and --workers N runs N, which shortens "
          "the wait." % DEFAULT_WORKERS)
    if input("Continue? [Y/n] ").strip().lower() in ("n", "no"):
        raise SystemExit("Stopped before running anything.")


def timing(name, durations, elapsed, args):
    """What the run spent its time on, so a timeout or a worker count can be
    chosen from measurement instead of guessed.

    Entry time is the sum of the entries, so it exceeds the elapsed time when
    workers run in parallel. The two together say what the parallelism bought.
    An entry that reaches the timeout is called out separately, because that
    is the part --timeout controls.
    """
    if not durations:
        return "%s: nothing ran" % name
    spent = sorted(durations.items(), key=lambda pair: -pair[1])
    total = sum(durations.values())
    # A worker is stopped by the timeout a shade after it expires, so compare
    # against slightly less than the limit rather than against it exactly.
    at_limit = [pair for pair in spent if pair[1] >= args.timeout * 0.95]
    # One fact per line under a label, because these numbers are read against
    # each other: wall against entry time says what the workers bought, and
    # entry time against the limit says where the time went.
    lines = ["%s stats:" % name,
             _row("elapsed", "%s with %s"
                  % (_span(elapsed),
                     _plural(args.workers, "worker", "workers"))),
             _row("entry time", "%s added up, %.1fs per entry on average"
                  % (_span(total), total / len(durations)))]
    if at_limit:
        lines.append(_row("at the limit",
                          "%d of %d entries used the full %gs, %s of that time"
                          % (len(at_limit), len(durations), args.timeout,
                             _span(sum(pair[1] for pair in at_limit)))))
    # The names are long, so one entry per line. Skipped when only one entry
    # ran, because the average above already reports its time.
    if len(spent) > 1:
        lines.append(_row("slowest", ""))
        lines.extend("      %5.1fs  %s" % (seconds, entry)
                     for entry, seconds in spent[:SLOWEST_SHOWN])
    return "\n".join(lines)


def _row(label, text):
    if not text:
        return "  " + label
    return "  %-*s%s" % (LABEL_WIDTH, label, text)


def _plural(count, one, many):
    return "%d %s" % (count, one if count == 1 else many)


def _span(seconds):
    if seconds < 60:
        return "%.1fs" % seconds
    return "%dm%02ds" % (seconds // 60, seconds % 60)


def summary(name, passed, failed, skipped):
    """One line per layer. The total counts skipped entries, so the rate says
    how much of the suite actually runs, not how much of what ran passed."""
    total = passed + failed + skipped
    rate = (100.0 * passed / total) if total else 0.0
    return ("%s: %d passed, %d failed, %d skipped of %d (%.1f%%)"
            % (name, passed, failed, skipped, total, rate))


def classify(args, layer, entry):
    """Probe one entry and return ("active", "") or (category, detail)."""
    try:
        layer.run_one(args.starfish, entry, args.timeout)
    except (AssertionError, OSError, RuntimeError, ValueError) as error:
        if layer.name == "behavior":
            return behavior.reason(error)
        # The schema layer sends one command, so there is nothing to group by:
        # the response that failed the check is the whole story.
        return "failed", " ".join(str(error).split())
    return "active", ""


LAYER_HELP = {
    "schema": """\
Send each listed CDP command with no parameters and check that the response
carries every field the pinned protocol declares non-optional.

Broad and cheap, but it only checks shape. The protocol states no values, so
neither can this. It also cannot reach a command that takes a required
parameter, because the protocol does not say where such a value comes from.
""",
    "behavior": """\
Run Chromium's inspector-protocol tests unmodified and compare each test's
output with the -expected.txt that ships beside it.

The expectations come from the people who define CDP, not from us. The tests
are JavaScript, so this layer needs a Node binary; the schema layer does not.
""",
}

ALL_HELP = """\
Run every layer in order: schema, then behavior.

Each layer keeps its own list and its own totals, and one failing layer does
not stop the next. The exit status is non-zero if any layer failed.
"""

INCLUDE_FAILS_HELP = """\
also run the entries the list expects to fail, so one that now passes is
promoted back to expected-pass. It starts a browser per entry, so a full sweep
takes a while. Without it a run covers the expected-pass entries only, and can
only demote"""

EXAMPLES = """\
examples:
  run.py all                          both layers, then a combined total
  run.py behavior                     the entries expected to pass
  run.py behavior --include-fails     every entry, so one can be promoted
  run.py behavior 'dom/*.js'          one directory, whatever it expects
  run.py behavior access-inspected-object.js
                                      one entry, whatever it expects
  run.py behavior --list subset.json  the entries that file names

  Exit status is 0 unless an entry expected to pass failed.
"""


def add_pattern(parser):
    parser.add_argument(
        "pattern", nargs="?", metavar="PATTERN",
        help="run the entries whose name matches this shell pattern, "
             "whatever the list expects of them. A name is a CDP method "
             "('DOM.getDocument') for the schema layer, or a path under the "
             "suite root ('dom/resolve-node-blocked.js') for the behavior "
             "layer. No match is an error")


def add_shared_options(parser):
    parser.add_argument(
        "--starfish", metavar="PATH", type=pathlib.Path,
        help="the Starfish binary to test. When omitted, the CDP-enabled "
             "build under out/ is used, preferring a headless one. Anything "
             "other than a single match is an error that names the "
             "candidates")
    parser.add_argument(
        "--list", metavar="PATH", type=pathlib.Path,
        help="run the entries this file names, whatever the layer's own list "
             "expects of them. Results still go into the layer's own list")
    parser.add_argument(
        "--include-fails", action="store_true", help=INCLUDE_FAILS_HELP)
    parser.add_argument(
        # Three settings from one flag: absent, bare, and with a number. The
        # default stays None so a run can tell "not asked for" from "asked
        # for 1", and only remind the first.
        "--workers", metavar="N", type=int, nargs="?",
        const=DEFAULT_WORKERS, default=None,
        help="how many entries to run at once, each in its own browser on "
             "its own port. Without this flag entries run one at a time; "
             "--workers on its own runs %d at a time; --workers N runs N. "
             "Entries are independent, so this only shortens the elapsed "
             "time, but a loaded machine is slower per entry and can turn a "
             "slow entry into a timeout" % DEFAULT_WORKERS)
    parser.add_argument(
        "--timeout", metavar="SECONDS", type=float,
        help="how long one entry may take, browser start-up included "
             "(default 10, or 5 with --include-fails). An entry that times "
             "out costs the full value")
    parser.add_argument(
        "--virtual-display", action="store_true",
        help="run the browser under xvfb-run on a throwaway 1920x1080x24 "
             "screen, which needs xvfb-run on PATH. An x11 build needs this "
             "unless DISPLAY is already set; a headless build never does")


def main():
    parser = argparse.ArgumentParser(
        description=__doc__.strip(), epilog=EXAMPLES,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    subcommands = parser.add_subparsers(
        dest="command", required=True, metavar="COMMAND")

    schema = subcommands.add_parser(
        "schema", description=LAYER_HELP["schema"],
        formatter_class=argparse.RawDescriptionHelpFormatter,
        help="check that responses carry the fields the protocol declares")
    behavior_parser = subcommands.add_parser(
        "behavior", description=LAYER_HELP["behavior"],
        formatter_class=argparse.RawDescriptionHelpFormatter,
        help="run Chromium's inspector-protocol tests unmodified")
    for name, layer_parser in (("schema", schema),
                               ("behavior", behavior_parser)):
        add_shared_options(layer_parser)
        add_pattern(layer_parser)
        layer_parser.set_defaults(handler=run_layer, layer=name,
                                  default_timeout=10)

    every = subcommands.add_parser(
        "all", description=ALL_HELP,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        help="run every layer in order")
    add_shared_options(every)
    add_pattern(every)
    every.set_defaults(handler=None, layer=None, default_timeout=10)

    args = parser.parse_args()
    args.workers_given = args.workers is not None
    if not args.workers_given:
        args.workers = 1
    if args.workers < 1:
        raise SystemExit("--workers must be at least 1")
    if args.workers > VERIFIED_WORKERS:
        # Not refused: a bigger machine may well take it. Said out loud
        # because past this point nobody has checked that the entries still
        # get their time, and a starved entry is recorded as a timeout.
        print("WARNING: %d workers is past the %d this was tried with; "
              "watch for entries that time out only under load"
              % (args.workers, VERIFIED_WORKERS))
    if args.include_fails:
        # A sweep pays the timeout on every unsupported entry, so it waits
        # less per entry than a run over the active ones alone.
        args.default_timeout = 5
    if args.timeout is None:
        args.timeout = args.default_timeout

    # Arm cleanup before the first browser starts, and clear anything a run
    # that was killed outright left behind.
    launcher.virtual_display = args.virtual_display
    launcher.install_cleanup()
    report_strays(launcher.sweep_strays(), "an earlier run")
    stale_storage = launcher.sweep_storage()
    if stale_storage:
        print("Removed the browser storage of %d run(s) that were killed"
              % stale_storage)

    if args.starfish is None:
        args.starfish = launcher.find_starfish()
    elif not args.starfish.is_file():
        # Every entry would otherwise fail on the same missing binary and the
        # list would be rewritten with that error as each entry's detail.
        raise SystemExit("No such binary: %s" % args.starfish)
    try:
        if args.command == "all":
            return run_every_layer(args)
        result = args.handler(args, LAYERS[args.layer](args))
        # run_layer also returns its counts, which only "all" needs.
        return result[0] if isinstance(result, tuple) else result
    finally:
        # Sweep again on the way out: a browser that escaped its own stop()
        # must not outlive the run that started it.
        report_strays(launcher.sweep_strays(), "this run")


def run_every_layer(args):
    """Run both layers. One failing layer must not hide the other's result."""
    status = 0
    totals = [0, 0, 0]
    for name in ("schema", "behavior"):
        print("== %s ==" % name)
        code, counts = run_layer(args, LAYERS[name](args))
        status |= code
        totals = [a + b for a, b in zip(totals, counts)]
        print()
    print(summary("total", *totals))
    return status


def report_strays(count, whose):
    if count:
        print("Killed %d browser(s) left behind by %s" % (count, whose))


if __name__ == "__main__":
    sys.exit(main())
