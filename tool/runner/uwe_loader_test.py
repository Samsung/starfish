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
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""End-to-end scenarios for the UWE (Updatable Web Engine) dynamic loader.

With -DENABLE_DYNAMIC_LOADER=1 the API .so (libStarfish.so) picks its impl
.so at runtime, preferring a separately-delivered updated copy over the one
installed with the platform when the updated copy's VERSION file reads
higher (LWELoaderUtils::openLWELibrary()). Ordinary CI already builds and
runs in loader mode, so the *default* branch of that choice is covered --
but nothing exercised the update branch, which is the whole point of UWE.

That branch is reachable from a plain host build without a second impl
build: build/starfish_public_api.cmake sets STARFISH_API_UWE_MOUNT_PATH to
"./update/" and STARFISH_API_DEFAULT_PATH to "./" on a linux host, both
resolved against the process's *current working directory* rather than the
binary's location, and MiniBrowser::init() already calls
LWE::LWE::SetVersionPreference(true) before LWE::LWE::Initialize(). So this
script just plants files under <repo-root>/update/ and re-runs ./Starfish
from the repo root.

Each entry in SCENARIOS sets up that directory, then asserts on the loader's
own stdout/stderr plus a clean exit. Run from repo root:

    xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/uwe_loader_test.py
"""

import os
import shlex
import shutil
import sys
import tempfile
from subprocess import PIPE, Popen

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from repo_paths import REPO_ROOT  # noqa: E402

# The two paths the loader resolves against the CWD, and therefore the two
# the scenarios below create and delete outright.
UPDATE_DIR = os.path.join(REPO_ROOT, "update")
DEFAULT_VERSION_PATH = os.path.join(REPO_ROOT, "VERSION")


def find_impl_so():
    """Resolve libStarfish-impl.so next to whatever ./Starfish points at.

    Deriving this from the ./Starfish symlink (rather than hardcoding a
    backend/mode path like out/x11/glib_cairo_gl/debug) keeps the script
    working across build configurations.
    """
    starfish_link = os.path.join(REPO_ROOT, "Starfish")
    real_bin = os.path.realpath(starfish_link)
    lib_dir = os.path.normpath(os.path.join(os.path.dirname(real_bin), "..", "lib"))
    impl_so = os.path.join(lib_dir, "libStarfish-impl.so")
    if not os.path.exists(impl_so):
        sys.exit("Cannot find libStarfish-impl.so next to %s -- build with "
                 "-DENABLE_DYNAMIC_LOADER=1 first." % starfish_link)
    return impl_so


def write_version(path, version):
    with open(path, "w") as f:
        f.write(version + "\n")


def reset_mount_state():
    if os.path.isdir(UPDATE_DIR):
        shutil.rmtree(UPDATE_DIR)
    if os.path.exists(DEFAULT_VERSION_PATH):
        os.remove(DEFAULT_VERSION_PATH)


def ensure_clean_start():
    """Bail if the paths the scenarios delete already exist.

    Neither is produced by a build -- the build writes its own VERSION under
    out/<...>/lib/, not here -- so anything sitting at these paths belongs to
    whoever is running this. Refuse rather than rmtree it.
    """
    for path in (UPDATE_DIR, DEFAULT_VERSION_PATH):
        if os.path.exists(path):
            sys.exit("%s already exists and this script would delete it.\n"
                     "Move it aside and re-run." % path)


def setup_updated_impl_selected(impl_so):
    # update/ holds a real impl with a higher VERSION -- it wins over the
    # default, and the default is never opened at all.
    os.makedirs(UPDATE_DIR)
    shutil.copy(impl_so, os.path.join(UPDATE_DIR, "libStarfish-impl.so"))
    write_version(os.path.join(UPDATE_DIR, "VERSION"), "9.9.9")


def setup_lower_version_ignored(impl_so):
    # update/ holds a real impl, but an older one than the default claims to
    # be -- the loader must not even try it.
    os.makedirs(UPDATE_DIR)
    shutil.copy(impl_so, os.path.join(UPDATE_DIR, "libStarfish-impl.so"))
    write_version(os.path.join(UPDATE_DIR, "VERSION"), "0.0.1")
    write_version(DEFAULT_VERSION_PATH, "9.9.9")


def setup_unloadable_file_falls_back(impl_so):
    # update/ advertises a higher version but the file isn't a loadable .so;
    # dlopen() must fail and the loader must fall back rather than give up.
    os.makedirs(UPDATE_DIR)
    with open(os.path.join(UPDATE_DIR, "libStarfish-impl.so"), "wb") as f:
        f.write(b"not an ELF file\n")
    write_version(os.path.join(UPDATE_DIR, "VERSION"), "9.9.9")


def build_fake_updated_impl(source):
    source_path = os.path.join(UPDATE_DIR, "fake_updated_impl.cpp")
    output_path = os.path.join(UPDATE_DIR, "libStarfish-impl.so")
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
        sys.exit("Cannot build fake UWE test library: " +
                 stderr.decode("utf-8", "replace"))
    write_version(os.path.join(UPDATE_DIR, "VERSION"), "9.9.9")


def setup_incompatible_abi_epoch_falls_back(impl_so):
    # A valid shared object with an incompatible ABI epoch must be rejected
    # before any ProcTable or vtable call is possible.
    build_fake_updated_impl(
        '#include "src/public/contract/LWEDelegateContract.h"\n'
        'extern "C" uint32_t LWEDelegate_GetAbiEpoch()\n'
        '{ return LWEDelegate::kDelegateAbiEpoch + 1; }\n')


def setup_missing_abi_epoch_falls_back(impl_so):
    # A valid shared object without the handshake symbol must be rejected.
    build_fake_updated_impl(
        'extern "C" int UnrelatedSymbol() { return 0; }\n')


def setup_missing_symbol_falls_back(impl_so):
    # This candidate passes the version handshake but lacks every required
    # ProcTable symbol. A post-dlopen validation failure must also fall back.
    build_fake_updated_impl(
        '#include "src/public/contract/LWEDelegateContract.h"\n'
        'extern "C" uint32_t LWEDelegate_GetAbiEpoch()\n'
        '{ return LWEDelegate::kDelegateAbiEpoch; }\n')


SCENARIOS = [
    {
        "name": "updated_impl_selected",
        "setup": setup_updated_impl_selected,
        # No "Failed to load updated" and no attempt at the default is what
        # actually proves the updated copy is the one that got loaded -- the
        # loader stays quiet on the success path.
        "must_contain": ["Try to load updated LWE"],
        "must_not_contain": ["Failed to load updated LWE",
                             "Try to load default LWE"],
    },
    {
        "name": "lower_version_ignored",
        "setup": setup_lower_version_ignored,
        "must_contain": ["Try to load default LWE"],
        "must_not_contain": ["Try to load updated LWE"],
    },
    {
        "name": "unloadable_file_falls_back_to_default",
        "setup": setup_unloadable_file_falls_back,
        "must_contain": ["Try to load updated LWE",
                         "Failed to load updated LWE",
                         "Try to load default LWE"],
        "must_not_contain": ["Failed to load default LWE"],
    },
    {
        "name": "incompatible_abi_epoch_falls_back_to_default",
        "setup": setup_incompatible_abi_epoch_falls_back,
        "must_contain": ["Try to load updated LWE",
                         "LWE delegate ABI epoch mismatch",
                         "Updated LWE validation failed",
                         "Try to load default LWE"],
        "must_not_contain": ["Failed to load default LWE"],
    },
    {
        "name": "missing_abi_epoch_falls_back_to_default",
        "setup": setup_missing_abi_epoch_falls_back,
        "must_contain": ["Try to load updated LWE",
                         "LWE delegate ABI epoch symbol is missing",
                         "Updated LWE validation failed",
                         "Try to load default LWE"],
        "must_not_contain": ["Failed to load default LWE"],
    },
    {
        "name": "missing_symbol_falls_back_to_default",
        "setup": setup_missing_symbol_falls_back,
        "must_contain": ["Try to load updated LWE",
                         "Updated LWE validation failed",
                         "Try to load default LWE"],
        "must_not_contain": ["LWE delegate ABI epoch mismatch",
                             "Failed to load default LWE"],
    },
]


def run_starfish(shot_path):
    # --screen-shot is how the shell is told to render one frame and quit:
    # MiniBrowser::parseArgs() turns it into EXIT_AFTER_SCREEN_SHOT, which
    # WebView.cpp acts on after onload. Without it the browser window just
    # stays up and this never returns. The image itself is not wanted, so
    # it is written outside the repo.
    args = ["about:blank", "--screen-shot=" + shot_path]
    process = Popen(["./Starfish"] + args, stdout=PIPE, stderr=PIPE,
                    cwd=REPO_ROOT)
    stdout, stderr = process.communicate(timeout=60)
    return process.returncode, stdout.decode("utf-8", "replace") + \
        stderr.decode("utf-8", "replace")


def run_scenario(scenario, impl_so, shot_path):
    reset_mount_state()
    try:
        scenario["setup"](impl_so)
        exit_code, output = run_starfish(shot_path)
    finally:
        reset_mount_state()

    failures = []
    if exit_code != 0:
        failures.append("exited with code %d (expected 0)" % exit_code)
    for needle in scenario["must_contain"]:
        if needle not in output:
            failures.append("missing expected output: %r" % needle)
    for needle in scenario["must_not_contain"]:
        if needle in output:
            failures.append("unexpected output present: %r" % needle)

    if failures:
        print("[FAIL] %s" % scenario["name"])
        for f in failures:
            print("    - %s" % f)
        print("  --- captured output ---")
        print("  " + output.replace("\n", "\n  "))
        return False

    print("[PASS] %s" % scenario["name"])
    return True


def main():
    impl_so = find_impl_so()
    ensure_clean_start()

    scratch = tempfile.mkdtemp(prefix="uwe_loader_test.")
    try:
        shot_path = os.path.join(scratch, "shot.png")
        results = [run_scenario(s, impl_so, shot_path) for s in SCENARIOS]
    finally:
        reset_mount_state()
        shutil.rmtree(scratch, ignore_errors=True)

    print()
    print("%d/%d scenarios passed." % (sum(results), len(results)))
    return 0 if all(results) else 1


if __name__ == "__main__":
    sys.exit(main())
