#!/usr/bin/env python3
# Copyright (c) 2026-present Samsung Electronics Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

"""Exercise test-built engine layers without linking the API or impl.

Compile-time probes are absent from ordinary Release builds. The driver
checks actual unmapping, mutable global/static reset, normal thread reuse,
explicit shutdown, and DOM/JS after every reload.
"""

import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


def make_legacy_impl(temp, name, compiler):
    """A pre-shutdown contract: only lifecycle wrappers are ever called."""
    root = Path(__file__).resolve().parents[2]
    headers = root / "src/public/contract"
    wrappers = set()
    for header in headers.glob("*.h"):
        wrappers.update(re.findall(r"\b(LWE(?:Worker)?Delegate_\w+)\s*\(",
                                   header.read_text()))
    implementations = {
        "LWEDelegate_GetAbiEpoch":
            'extern "C" uint32_t LWEDelegate_GetAbiEpoch() '
            '{ return LWEDelegate::kDelegateAbiEpoch; }',
        "LWEDelegate_LWE_Initialize":
            'extern "C" void LWEDelegate_LWE_Initialize(const char*, uint32_t) '
            '{ initialized = true; }',
        "LWEDelegate_LWE_IsInitialized":
            'extern "C" bool LWEDelegate_LWE_IsInitialized() '
            '{ return initialized; }',
        "LWEDelegate_LWE_Finalize":
            'extern "C" void LWEDelegate_LWE_Finalize() '
            '{ initialized = false; }',
    }
    wrappers.discard("LWEDelegate_LWE_ShutdownForUnload")
    source = '#include "public/contract/LWEDelegateContract.h"\n'
    source += 'static bool initialized;\n'
    for wrapper in sorted(wrappers):
        source += implementations.get(
            wrapper, 'extern "C" void ' + wrapper + '() {}') + '\n'
    source_path = Path(temp) / "legacy.cpp"
    source_path.write_text(source)
    impl = Path(temp) / name
    subprocess.run(shlex.split(compiler) + [
        "-std=c++11", "-fPIC", "-shared", "-I", str(root / "src"),
        str(source_path), "-o", str(impl)], check=True)
    return impl


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--impl", required=True, type=Path)
    parser.add_argument("--api", type=Path,
                        help="also test public Finalize and API SO unloading")
    parser.add_argument("--cycles", type=int, default=5)
    parser.add_argument("--compiler", default=os.environ.get("CXX", "c++"))
    parser.add_argument("--incremental", action="store_true")
    parser.add_argument("--wait-for-worker", action="store_true",
                        help="verify shutdown waits for running finite JS")
    parser.add_argument("--host-thread", action="store_true",
                        help="unload/reload on the same GLib host thread")
    parser.add_argument("--legacy", action="store_true",
                        help="with --api, retain an impl without shutdown")
    args = parser.parse_args()
    if args.cycles < 1:
        parser.error("--cycles must be positive")
    if args.legacy and (not args.api or args.host_thread or
                        args.wait_for_worker):
        parser.error("--legacy requires --api and excludes host/worker tests")
    impl = args.impl.resolve(strict=True)
    flags = shlex.split(subprocess.check_output(
        ["pkg-config", "--cflags", "--libs", "glib-2.0"], text=True))
    with tempfile.TemporaryDirectory(prefix="starfish-impl-unload-") as temp:
        if args.legacy:
            impl = make_legacy_impl(temp, impl.name, args.compiler)
        # Real files avoid coupling lifecycle coverage to blob/data URL support.
        fixtures = {
            "unload-page.html": "<!doctype html><title>Unload test</title>",
            "unload-close.js": "close();",
            "unload-child.js": "postMessage(1);",
            "unload-parent.js": (
                "var child = new Worker('unload-child.js');"
                "child.onmessage = function() { postMessage(1); };"),
        }
        if args.wait_for_worker:
            # The child is still evaluating its initial script, and its parent
            # is executing a message callback, when document disposal starts.
            # Both must finish naturally after the former 1-second deadline.
            fixtures["unload-child.js"] = (
                "var deadline = Date.now() + 2000; postMessage(deadline);"
                "while (Date.now() < deadline) {}")
            fixtures["unload-parent.js"] = (
                "var child = new Worker('unload-child.js');"
                "child.onmessage = function(e) { postMessage(e.data);"
                "while (Date.now() < e.data) {} };")
        for name, source in fixtures.items():
            (Path(temp) / name).write_text(source)
        executable = Path(temp) / "impl-unload-test"
        subprocess.run(shlex.split(args.compiler) + [
            "-std=c++11", "-pthread", "-Wall", "-Wextra", "-Werror",
            str(Path(__file__).with_suffix(".cpp")), "-o", str(executable),
            "-ldl"] + flags, check=True)
        env = os.environ.copy()
        env["LD_LIBRARY_PATH"] = str(impl.parent) + os.pathsep + env.get(
            "LD_LIBRARY_PATH", "")
        if args.incremental:
            env["GC_ENABLE_INCREMENTAL"] = "1"
        else:
            env.pop("GC_ENABLE_INCREMENTAL", None)
        mode = "legacy" if args.legacy else (
            "host" if args.host_thread else "dedicated")
        command = [str(executable), str(impl), temp, str(args.cycles), mode]
        if args.api:
            command.append(str(args.api.resolve(strict=True)))
        if args.wait_for_worker:
            result = subprocess.run(command, env=env,
                                    timeout=180 + 4 * args.cycles,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, text=True)
            print(result.stdout, end="")
            result.check_returncode()
            waits = re.findall(
                r"Worker shutdown waited (\d+) ms \(JS remaining (\d+) ms\)",
                result.stdout)
            if not any(int(remaining) > 1000 for _, remaining in waits):
                raise RuntimeError("test did not observe running JS past 1s")
        else:
            subprocess.run(command, env=env, check=True, timeout=180)


if __name__ == "__main__":
    main()
