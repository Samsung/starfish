# Copyright (c) 2026-present Samsung Electronics Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

"""Check Memcheck loss roots against the known Ubuntu 24.04 unload debt."""

from collections import Counter
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET


# Bytes include all indirect allocations attached to each definite loss root.
# Bound each known stack family separately per complete IMPL unload.
# Fontconfig and libgomp use the maxima measured on both the Docker
# reproduction host and the x64 CI runners.
KNOWN_BYTES_PER_UNLOAD = {
    "Pixman initialization": 12384,
    "Fontconfig parser and defaults": 339,
    "Cairo drawing caches": 1976,
    "Starfish FreeType library": 2464,
    "libgomp initialization": 112,
    "libssh-gcrypt initialization": 192,
}


def known_root(frames):
    """Recognize only measured stacks from the current CI dependencies."""
    objects = {obj for obj, _ in frames}
    functions = {fn for _, fn in frames}
    allocator, library = frames[0][1], frames[1][0]
    loader = {"call_init", "_dl_init"}.issubset(functions)
    if loader and allocator == "calloc" and library == "libpixman-1.so.0.42.2":
        return "Pixman initialization"
    if loader and allocator in ("malloc", "calloc") and library == "libgomp.so.1.0.0":
        return "libgomp initialization"
    if (loader and allocator == "malloc" and library == "libgcrypt.so.20.4.3"
            and "libssh-gcrypt.so.4.9.6" in objects and "gcry_check_version" in functions):
        return "libssh-gcrypt initialization"
    # The same lost FreeType library can be rooted at the library object
    # or at one of its default modules when the process unloads repeatedly.
    if (allocator == "malloc" and library == "libfreetype.so.6.20.1"
            and "FT_Init_FreeType" in functions
            and functions.intersection({"FT_New_Library", "FT_Add_Default_Modules"})
            and "Starfish::PlatformFontSelectorImplCairo::PlatformFontSelectorImplCairo" in functions):
        return "Starfish FreeType library"

    if allocator == "malloc" and library == "libcairo.so.2.11800.0":
        cairo_calls = {
            ("cairo_clip", "Starfish::CanvasCairo::pixelSnappedClip"),
            ("cairo_save", "Starfish::CanvasCairo::save"),
            ("cairo_save", "Starfish::CanvasCairo::clearColor"),
            ("cairo_paint", "Starfish::CanvasCairo::clearColor"),
            ("cairo_pattern_create_rgba", "Starfish::CanvasCairo::clearColor"),
        }
        if (("Starfish::CanvasCairo::initFromBuffer" == frames[2][1])
                or any(call in functions and owner in functions
                       for call, owner in cairo_calls)):
            return "Cairo drawing caches"

    if allocator == "malloc" and "libfontconfig.so.1.12.1" in objects:
        # FcConfigCreate is an owned config, so its loss must always fail.
        if "FcConfigCreate" in functions:
            return None
        if library == "libfontconfig.so.1.12.1" or frames[1] == ("libc.so.6", "strdup"):
            if (("XML_ParseBuffer" in functions or "FcConfigBuildFonts" in functions)
                    or ("Starfish::PlatformFontSelectorImplCairo::findFont" in functions
                        and functions.intersection({"FcDefaultSubstitute",
                                                    "FcGetDefaultLangs",
                                                    "FcConfigSubstituteWithPat"}))):
                return "Fontconfig parser and defaults"
    return None


def verify_report(report, cycles):
    known = Counter()
    failed = False
    if report.findtext("status[last()]/state") != "FINISHED":
        raise RuntimeError("Memcheck did not finish the unload test")
    for error in report.findall("error"):
        kind = error.findtext("kind")
        frames = [(Path(frame.findtext("obj", "")).name,
                   frame.findtext("fn", "?").split("(", 1)[0])
                  for frame in error.findall("stack/frame")]
        if kind == "Leak_DefinitelyLost":
            root = known_root(frames) if len(frames) >= 3 else None
            if root:
                known[root] += int(error.findtext("xwhat/leakedbytes"))
                continue
        failed = True
        print("Memcheck unexpected:", error.findtext("xwhat/text") or error.findtext("what") or kind)
        for obj, fn in frames:
            print(f"  {fn} [{obj}]")

    for root, amount in sorted(known.items()):
        limit = KNOWN_BYTES_PER_UNLOAD[root] * cycles
        if amount > limit:
            failed = True
            print(f"Memcheck regression: {root}: {amount} bytes exceeds {limit} bytes")
        else:
            print(f"Memcheck known debt: {root}: {amount}/{limit} bytes")
    if failed:
        raise RuntimeError(f"Memcheck found an unload regression after {cycles} cycles")
    print(f"Memcheck: no unexpected allocation errors or loss roots after {cycles} cycles", flush=True)


def check_unloads(command, env, temp, fixtures):
    for cycles in (1, 5):
        storage = Path(temp) / f"memcheck-{cycles}"
        storage.mkdir()
        for name, source in fixtures.items():
            (storage / name).write_text(source)
        output = storage / "memcheck.xml"
        test = command.copy()
        test[2:4] = [str(storage), str(cycles)]
        print(f"Memcheck: running {cycles} unload cycles", flush=True)
        # Conservative GC reads native stack slots, including uninitialized
        # padding. Keep the other Memcheck diagnostics and all definite roots.
        result = subprocess.run([
            "valgrind", "--tool=memcheck", "--undef-value-errors=no",
            "--keep-debuginfo=yes", "--leak-check=full",
            "--show-leak-kinds=definite", "--num-callers=24",
            "--xml=yes", f"--xml-file={output}", *test,
        ], env=env, timeout=300)
        if output.exists():
            verify_report(ET.parse(output).getroot(), cycles)
        elif result.returncode == 0:
            raise RuntimeError("Memcheck did not produce an unload report")
        result.check_returncode()
