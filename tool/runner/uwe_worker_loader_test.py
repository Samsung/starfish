#!/usr/bin/env python3

# Copyright (c) 2026-present Samsung Electronics Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""End-to-end UWE loader scenarios for SharedWorker and ServiceWorker.

Build both worker hosts with ENABLE_DYNAMIC_LOADER=1, then run from the repo
root:

    xvfb-run -s '-screen 0 1920x1080x24' -a \
        ./tool/runner/uwe_worker_loader_test.py

Each scenario starts a worker host, waits for it to initialize through either
the updated or fallback impl, and interrupts it for a clean shutdown.
"""

import os
import shlex
import shutil
import signal
import sys
import tempfile
import time
from subprocess import PIPE, STDOUT, Popen, TimeoutExpired

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from repo_paths import REPO_ROOT  # noqa: E402


UPDATE_DIR = os.path.join(REPO_ROOT, "update")
DEFAULT_VERSION_PATH = os.path.join(REPO_ROOT, "VERSION")

WORKERS = [
    {
        "name": "sharedworker",
        "binary": "Starfish-sharedworker",
        "library": "libStarfish-sharedworker-impl.so",
    },
    {
        "name": "serviceworker",
        "binary": "Starfish-serviceworker",
        "library": "libStarfish-serviceworker-impl.so",
    },
]


def write_version(path, version):
    with open(path, "w") as f:
        f.write(version + "\n")


def reset_mount_state():
    if os.path.isdir(UPDATE_DIR):
        shutil.rmtree(UPDATE_DIR)
    if os.path.exists(DEFAULT_VERSION_PATH):
        os.remove(DEFAULT_VERSION_PATH)


def ensure_clean_start():
    for path in (UPDATE_DIR, DEFAULT_VERSION_PATH):
        if os.path.exists(path):
            sys.exit("%s already exists and this script would delete it.\n"
                     "Move it aside and re-run." % path)


def find_worker_impl(worker):
    binary_path = os.path.join(REPO_ROOT, worker["binary"])
    real_binary = os.path.realpath(binary_path)
    impl_path = os.path.normpath(
        os.path.join(os.path.dirname(real_binary), "..", "lib",
                     worker["library"]))
    if not os.path.exists(impl_path):
        sys.exit("Cannot find %s next to %s -- build the worker hosts with "
                 "-DENABLE_DYNAMIC_LOADER=1 first." %
                 (worker["library"], binary_path))
    return impl_path


def setup_updated_impl_selected(worker, impl_path):
    os.makedirs(UPDATE_DIR)
    shutil.copy(impl_path, os.path.join(UPDATE_DIR, worker["library"]))
    write_version(os.path.join(UPDATE_DIR, "VERSION"), "9.9.9")


def build_fake_updated_impl(worker, source):
    source_path = os.path.join(UPDATE_DIR, "fake_updated_impl.cpp")
    output_path = os.path.join(UPDATE_DIR, worker["library"])
    os.makedirs(UPDATE_DIR)
    with open(source_path, "w") as f:
        f.write(source)

    compiler = shlex.split(os.environ.get("CXX", "c++"))
    process = Popen(compiler + ["-std=c++11", "-shared", "-fPIC",
                               "-I", REPO_ROOT, source_path,
                               "-o", output_path],
                    stdout=PIPE, stderr=PIPE)
    _, stderr = process.communicate()
    if process.returncode != 0:
        sys.exit("Cannot build fake UWE worker library: " +
                 stderr.decode("utf-8", "replace"))
    write_version(os.path.join(UPDATE_DIR, "VERSION"), "9.9.9")


def setup_incompatible_abi_epoch(worker, impl_path):
    build_fake_updated_impl(
        worker,
        '#include "src/public/contract/LWEDelegateContract.h"\n'
        'extern "C" uint32_t LWEDelegate_GetAbiEpoch()\n'
        '{ return LWEDelegate::kDelegateAbiEpoch + 1; }\n')


def setup_missing_abi_epoch(worker, impl_path):
    build_fake_updated_impl(
        worker, 'extern "C" int UnrelatedSymbol() { return 0; }\n')


def setup_missing_worker_symbol(worker, impl_path):
    build_fake_updated_impl(
        worker,
        '#include "src/public/contract/LWEDelegateContract.h"\n'
        'extern "C" uint32_t LWEDelegate_GetAbiEpoch()\n'
        '{ return LWEDelegate::kDelegateAbiEpoch; }\n')


SCENARIOS = [
    {
        "name": "updated_impl_selected",
        "setup": setup_updated_impl_selected,
        "must_contain": ["Try to load updated LWE", "WORKER STARTS"],
        "must_not_contain": ["Failed to load updated LWE",
                             "Try to load default LWE"],
    },
    {
        "name": "incompatible_abi_epoch_falls_back_to_default",
        "setup": setup_incompatible_abi_epoch,
        "must_contain": ["Try to load updated LWE",
                         "LWE delegate ABI epoch mismatch",
                         "Updated LWE worker validation failed",
                         "Try to load default LWE", "WORKER STARTS"],
        "must_not_contain": ["Failed to load default LWE"],
    },
    {
        "name": "missing_abi_epoch_falls_back_to_default",
        "setup": setup_missing_abi_epoch,
        "must_contain": ["Try to load updated LWE",
                         "LWE delegate ABI epoch symbol is missing",
                         "Updated LWE worker validation failed",
                         "Try to load default LWE", "WORKER STARTS"],
        "must_not_contain": ["Failed to load default LWE"],
    },
    {
        "name": "missing_worker_symbol_falls_back_to_default",
        "setup": setup_missing_worker_symbol,
        "must_contain": ["Try to load updated LWE",
                         "Updated LWE worker validation failed",
                         "Try to load default LWE", "WORKER STARTS"],
        "must_not_contain": ["LWE delegate ABI epoch mismatch",
                             "Failed to load default LWE"],
    },
]


def run_worker(worker, data_dir):
    process = Popen(["./" + worker["binary"], "--data-dir=" + data_dir],
                    stdout=PIPE, stderr=STDOUT, cwd=REPO_ROOT,
                    start_new_session=True)
    time.sleep(2)

    exited_early = process.poll() is not None
    if not exited_early:
        os.killpg(os.getpgid(process.pid), signal.SIGINT)

    try:
        output, _ = process.communicate(timeout=30)
    except TimeoutExpired:
        os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        output, _ = process.communicate()
        return None, output.decode("utf-8", "replace"), False

    return process.returncode, output.decode("utf-8", "replace"), \
        exited_early


def run_scenario(worker, scenario, impl_path, scratch):
    reset_mount_state()
    data_dir = os.path.join(scratch, worker["name"] + "-data")
    os.makedirs(data_dir)
    try:
        scenario["setup"](worker, impl_path)
        exit_code, output, exited_early = run_worker(worker, data_dir)
    finally:
        reset_mount_state()
        shutil.rmtree(data_dir, ignore_errors=True)

    failures = []
    if exited_early:
        failures.append("worker exited before the test interrupted it")
    if exit_code is None:
        failures.append("worker did not exit after SIGINT")
    elif exit_code != 0:
        failures.append("exited with code %s (expected 0)" % exit_code)
    for needle in scenario["must_contain"]:
        if needle not in output:
            failures.append("missing expected output: %r" % needle)
    for needle in scenario["must_not_contain"]:
        if needle in output:
            failures.append("unexpected output present: %r" % needle)

    label = "%s/%s" % (worker["name"], scenario["name"])
    if failures:
        print("[FAIL] %s" % label)
        for failure in failures:
            print("    - %s" % failure)
        print("  --- captured output ---")
        print("  " + output.replace("\n", "\n  "))
        return False

    print("[PASS] %s" % label)
    return True


def main():
    ensure_clean_start()
    impl_paths = {worker["name"]: find_worker_impl(worker)
                  for worker in WORKERS}

    scratch = tempfile.mkdtemp(prefix="uwe_worker_loader_test.")
    try:
        results = [
            run_scenario(worker, scenario, impl_paths[worker["name"]], scratch)
            for worker in WORKERS
            for scenario in SCENARIOS
        ]
    finally:
        reset_mount_state()
        shutil.rmtree(scratch, ignore_errors=True)

    print()
    print("%d/%d scenarios passed." % (sum(results), len(results)))
    return 0 if all(results) else 1


if __name__ == "__main__":
    sys.exit(main())
