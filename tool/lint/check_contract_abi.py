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

"""Gate ABI breaks in the UWE delegate contract (src/public/contract/*.h).

UWE lets an already-installed API .so (libStarfish.so) dlopen a later,
separately-built impl .so (libStarfish-impl.so). The two are never compiled
together, so the delegate contract headers are the only thing keeping an old
API .so and a new impl .so agreeing on vtable slot order, extern "C" wrapper
signatures, and the layout of enums/structs that cross the boundary by
value. Nothing has ever checked that agreement before: a reordered virtual
method compiles and passes every existing test, then crashes in the field
the first time a UWE update pairs it with an old API .so.

This script compiles tool/lint/contract_abi/contract_shim.cpp -- a tiny TU
that includes only the delegate contract headers -- twice: once against a
git base commit's copy of the headers, once against the current working
tree, with byte-identical compiler flags. It reads each compiled .so's
debug info with abidw and classifies the difference:

    unchanged  -- nothing observable changed. Silent success.
    append     -- only additions strictly after everything that already
                  existed: new vtable slots, new enumerators with fresh
                  values, new ProcTable members, or a whole newly tracked
                  interface. Safe: an old API .so never looked for the new
                  member, so it keeps working unmodified against the new
                  impl.
    breaking   -- anything else: a slot moved, a signature changed, a member
                  vanished, an enum value was renumbered, .... An old API
                  .so calling into a new impl .so under this contract is not
                  safe.

What to compare is not configured anywhere -- the set of tracked classes,
nested structs, ProcTables and enums is derived from each side's *own* header
text (see parse_contract_surface()). Deriving per side is what makes growing
or shrinking the contract a normal, classifiable event instead of a checker
config change: a class present only in the new side's headers is reported as
"newly tracked" (append), one present only in the base side's as removed
(breaking). Only the *judgment* comes from the compiler's real output via
abidw; the header text is used to decide what to look for and to
cross-check that everything expected was actually emitted.

A checked-in ABI snapshot was deliberately rejected as the comparison
baseline: whatever regenerates it (an --update flag, say) can be run in the
very same PR that introduces the break, so CI stays green while the
snapshot silently launders the break. Comparing against a git base commit
the PR cannot rewrite closes that: even the shim.cpp compiled for the base
side is read from the base commit, not from the working tree, so editing
the shim to under-cover the contract shows up as a class disappearing
between the two sides rather than as "no difference to report."

A "breaking" verdict fails the PR (exit 1) unless every item it found has a
matching line freshly added (relative to the same base) in
tool/lint/contract_abi/breaking_changes.md -- naming the exact mangled name /
struct member / enumerator / wrapper that changed, not just touching the
file. Only lines *added* since the base count: matching against the file's
full current text would make every once-merged waiver silently cover all
future breaks of the same key forever.

Usage:
    check_contract_abi.py                 # base vs current working tree
    check_contract_abi.py --base <ref>     # base vs current working tree
    check_contract_abi.py --verbose        # print every vtable slot checked
    check_contract_abi.py --verify-checker # run this script's OWN fixture
                                            # tests -- verifies the checker
                                            # itself works, not your change

Exit codes: 0 unchanged/append (or breaking-with-waiver), 1 breaking without
a waiver, 2 setup failure (missing tool, unresolvable base, a class the
headers declare but the corpus failed to emit, or a disagreement between the
structural check and abidiff -- never silently treated as a pass).
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from repo_paths import REPO_ROOT  # noqa: E402

CONTRACT_DIR = "src/public/contract"
# Headers outside src/public/contract/ that define types crossing the
# API/impl boundary by value (referenced from contract signatures).
EXTRA_HEADERS = ("inc/PlatformIntegrationData.h", "inc/LWEWorker.h")
SHIM_REL_PATH = "tool/lint/contract_abi/contract_shim.cpp"
WAIVER_REL_PATH = "tool/lint/contract_abi/breaking_changes.md"
LOADER_REL_PATHS = (
    "src/public/LWEDelegateLoader.cpp",
    "src/public/LWEWorkerDelegateLoader.cpp",
)
WITNESS_STRUCT = "ContractAbiWitness"

ABIDW_FLAGS = [
    "--load-all-types",
    "--type-id-style", "hash",
    "--no-show-locs",
    "--no-corpus-path",
    "--no-elf-needed",
    "--no-write-default-sizes",
    "--no-parameter-names",
    "--no-comp-dir-path",
]

COMPILE_FLAGS = [
    "-std=c++11", "-g3", "-fvisibility=hidden", "-femit-class-debug-always",
    "-DSTARFISH_WEBWORKER_HOST", "-shared", "-fPIC",
]

CLASS_RE = re.compile(r"class\s+EXPORT_UNMANAGED_API\s+(\w+)\s*\{")
NAMED_STRUCT_RE = re.compile(r"\bstruct\s+(\w+)\s*\{")
PROCTABLE_RE = re.compile(r"\b(\w+ProcTable)\s*;")
ENUM_RE = re.compile(r"\benum(?:\s+class)?(?:\s+LWE_EXPORT)?\s+(\w+)\s*[:{]")
VIRTUAL_RE = re.compile(r"\bvirtual\b")
WRAPPER_NAME_RE = re.compile(r"\b(LWE(?:Worker)?Delegate_[A-Za-z_]+)\s*\(")
DLSYM_RE = re.compile(r'dlsym\(\s*m_handle\s*,\s*"([A-Za-z_]+)"', re.S)
WRAPPER_CHECK_MACRO_RE = re.compile(r"\bCONTRACT_ABI_CHECK_WRAPPER\s*\(")


class SetupError(Exception):
    """Anything that must produce exit 2, never a silent pass or a BREAKING."""


# ---------------------------------------------------------------------------
# Tool discovery
# ---------------------------------------------------------------------------

def require_tool(name):
    path = shutil.which(name)
    if not path:
        raise SetupError(
            "%s not found on PATH -- install abigail-tools (see README.md)"
            % name)
    return path


def check_abidw_version():
    out = subprocess.run([require_tool("abidw"), "--version"],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT
                         ).stdout.decode("utf-8", "replace")
    m = re.search(r"(\d+)\.(\d+)", out)
    if not m or int(m.group(1)) < 2:
        raise SetupError(
            "abidw --version reported %r; need >= 2.0 (--type-id-style "
            "hash)" % out.strip())


# ---------------------------------------------------------------------------
# Extracting a header/shim tree from a git ref or the working tree
# ---------------------------------------------------------------------------

def git(*args):
    result = subprocess.run(["git", "-C", REPO_ROOT] + list(args),
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        raise SetupError("git %s failed: %s" %
                         (" ".join(args),
                          result.stderr.decode("utf-8", "replace").strip()))
    return result.stdout.decode("utf-8", "replace")


def resolve_ref(ref):
    return git("rev-parse", "--verify", ref + "^{commit}").strip()


def tracked_paths_at_ref(ref, *roots):
    out = git("ls-tree", "-r", "--name-only", ref, "--", *roots)
    return [line for line in out.splitlines() if line]


def export_ref_tree(ref, dest):
    """Write ref's copy of the contract headers + shim into dest/<relpath>."""
    paths = list(tracked_paths_at_ref(ref, CONTRACT_DIR))
    for extra in EXTRA_HEADERS + (SHIM_REL_PATH,):
        if extra in tracked_paths_at_ref(ref, extra):
            paths.append(extra)
        else:
            raise SetupError(
                "%s does not exist at %s -- cannot compare against a "
                "commit that predates this checker" % (extra, ref))
    for relpath in paths:
        content = git("show", "%s:%s" % (ref, relpath))
        target = os.path.join(dest, relpath)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        with open(target, "w") as f:
            f.write(content)
    return paths


def export_working_tree(dest):
    paths = []
    for fname in sorted(os.listdir(os.path.join(REPO_ROOT, CONTRACT_DIR))):
        if fname.endswith(".h"):
            paths.append(os.path.join(CONTRACT_DIR, fname))
    for extra in EXTRA_HEADERS + (SHIM_REL_PATH,):
        if not os.path.exists(os.path.join(REPO_ROOT, extra)):
            raise SetupError("%s is missing from the working tree" % extra)
        paths.append(extra)
    for relpath in paths:
        src = os.path.join(REPO_ROOT, relpath)
        target = os.path.join(dest, relpath)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        shutil.copyfile(src, target)
    return paths


# ---------------------------------------------------------------------------
# Deriving the tracked surface from header text.
#
# This is deliberately regex-based and deliberately NOT the judge: an
# earlier, fully regex-based checker was rejected because source text can't
# see what the compiler actually lays out. Here regex only answers "what
# names should the compiled corpus contain" (per side, from that side's own
# headers) -- misparse in either direction fails loudly: a name the regex
# finds but the corpus lacks is a SetupError, and a real contract type the
# regex misses would fail the corpus-vs-header virtual-count cross-check.
# ---------------------------------------------------------------------------

def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def find_matching_brace(text, open_index):
    depth = 0
    for i in range(open_index, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    raise SetupError("unmatched brace in a contract header")


def parse_contract_surface(tree_dir):
    """Return the tracked-name sets this side's headers declare.

    {"vtable_classes": {name: virtual-declaration count},
     "structs": set,       # nested by-value structs (conservative policy)
     "proctables": set,    # append-at-end allowed
     "enums": set}
    """
    surface = {"vtable_classes": {}, "structs": set(),
               "proctables": set(), "enums": set()}
    contract_dir = os.path.join(tree_dir, CONTRACT_DIR)
    for fname in sorted(os.listdir(contract_dir)):
        if not fname.endswith(".h"):
            continue
        text = strip_comments(open(os.path.join(contract_dir, fname)).read())
        for m in CLASS_RE.finditer(text):
            body_end = find_matching_brace(text, m.end() - 1)
            body = text[m.end():body_end]
            vcount = len(VIRTUAL_RE.findall(body))
            if vcount:
                surface["vtable_classes"][m.group(1)] = vcount
            for sm in NAMED_STRUCT_RE.finditer(body):
                surface["structs"].add(sm.group(1))
        surface["proctables"].update(PROCTABLE_RE.findall(text))
    for extra in EXTRA_HEADERS:
        text = strip_comments(open(os.path.join(tree_dir, extra)).read())
        surface["enums"].update(ENUM_RE.findall(text))
    return surface


def extract_wrapper_names(tree_dir):
    names = set()
    contract_dir = os.path.join(tree_dir, CONTRACT_DIR)
    for fname in os.listdir(contract_dir):
        if fname.endswith(".h"):
            text = open(os.path.join(contract_dir, fname)).read()
            names.update(WRAPPER_NAME_RE.findall(text))
    return names


# ---------------------------------------------------------------------------
# Compiling the shim and dumping its ABI
# ---------------------------------------------------------------------------

def compile_shim(tree_dir):
    """Compile tree_dir/tool/lint/contract_abi/contract_shim.cpp in place.

    Deliberately does NOT add -I<REPO_ROOT>/... to the include path: the
    only include directories are the two under tree_dir, so a quoted
    #include in a contract header (e.g. "PlatformIntegrationData.h",
    resolved first relative to the including file's own directory, then the
    -I list) can never silently resolve to a *different* copy of a header
    than the one exported into this same tree_dir -- which is exactly how
    the base side could otherwise pick up working-tree headers and make a
    real change invisible.
    """
    so_path = os.path.join(tree_dir, "shim.so")
    dep_path = os.path.join(tree_dir, "shim.d")
    cmd = (["g++"] + COMPILE_FLAGS +
          ["-I" + os.path.join(tree_dir, "src"),
           "-I" + os.path.join(tree_dir, "inc"),
           "-MD", "-MF", dep_path,
           "-o", so_path, SHIM_REL_PATH])
    result = subprocess.run(cmd, cwd=tree_dir,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        raise SetupError(
            "compiling %s failed:\n%s" %
            (SHIM_REL_PATH, result.stdout.decode("utf-8", "replace")))
    assert_no_include_leak(dep_path, tree_dir)
    return so_path


def assert_no_include_leak(dep_path, tree_dir):
    """The single most likely way this gate goes silently vacuous: a quoted
    #include resolving outside tree_dir (into the real REPO_ROOT) because
    some other -I ended up on the command line. Every header the compiler
    actually opened must live under tree_dir.
    """
    if not os.path.exists(dep_path):
        return
    text = open(dep_path).read().replace("\\\n", " ")
    tokens = text.split(":", 1)[1].split() if ":" in text else []
    real_root = os.path.realpath(REPO_ROOT) + os.sep
    real_tree = os.path.realpath(tree_dir) + os.sep
    for tok in tokens:
        # g++ was invoked with cwd=tree_dir, so a relative token in the
        # dependency file (e.g. the shim's own source path) is relative to
        # tree_dir, not to this script's own cwd -- os.path.realpath() alone
        # would resolve it against the wrong directory and false-positive
        # on every run.
        abs_tok = tok if os.path.isabs(tok) else os.path.join(tree_dir, tok)
        real_tok = os.path.realpath(abs_tok)
        if real_tok.startswith(real_root) and not real_tok.startswith(real_tree):
            raise SetupError(
                "include leak: %s resolved outside the isolated tree into "
                "%s -- the base/working-tree comparison is no longer "
                "trustworthy" % (tok, real_tok))


def dump_abi(so_path, xml_path):
    cmd = ["abidw"] + ABIDW_FLAGS + ["--out-file", xml_path, so_path]
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        raise SetupError("abidw failed on %s:\n%s" %
                         (so_path, result.stdout.decode("utf-8", "replace")))


# ---------------------------------------------------------------------------
# Fingerprint extraction
# ---------------------------------------------------------------------------

def find_decl(root, tag, name):
    """Find the (unique) fully-defined decl of `name` anywhere in the corpus.

    ElementTree's iter() is already recursive over all descendants, so
    nested types (LWEDelegate::WebContainer::RenderInfo lives inside a
    <member-type> inside its enclosing class-decl) are reached without any
    structural walking. Declaration-only stubs (forward declarations the
    compiler emitted alongside the real definition) are skipped. More than
    one distinct full definition for one name would make the comparison
    ambiguous, so that fails loudly rather than picking one.
    """
    hits = [el for el in root.iter(tag)
            if el.get("name") == name
            and el.get("is-declaration-only") != "yes"]
    if not hits:
        return None
    if len(hits) > 1:
        raise SetupError(
            "ambiguous corpus: %d full definitions of %s %r -- cannot "
            "compare reliably" % (len(hits), tag, name))
    return hits[0]


def extract_class_fp(node):
    slots = {}
    dtor = None
    for mf in node.iter("member-function"):
        fd = mf.find("function-decl")
        if fd is None:
            continue
        off = mf.get("vtable-offset")
        if off is None:
            continue
        mangled = fd.get("mangled-name")
        ret_el = fd.find("return")
        ret_id = ret_el.get("type-id") if ret_el is not None else None
        params = tuple(p.get("type-id") for p in fd.findall("parameter"))
        if mf.get("destructor") == "yes":
            dtor = (mangled, ret_id, params)
            continue
        ioff = int(off)
        if ioff < 0:
            continue
        slots[ioff] = (mangled, ret_id, params)
    return {
        "size_in_bits": node.get("size-in-bits"),
        "slots": slots,
        "dtor": dtor,
        # matches the header's own `virtual` declaration count 1:1
        # (destructor included) -- cross-checked in assert_coverage_sane.
        "virtual_count": len(slots) + (1 if dtor else 0),
    }


def extract_struct_fp(node):
    members = {}
    for dm in node.findall("data-member"):
        vd = dm.find("var-decl")
        if vd is None:
            continue
        members[vd.get("name")] = (dm.get("layout-offset-in-bits"),
                                   vd.get("type-id"))
    return {"size_in_bits": node.get("size-in-bits"), "members": members}


def extract_enum_fp(node):
    enumerators = [(e.get("name"), e.get("value"))
                  for e in node.findall("enumerator")]
    underlying = node.find("underlying-type")
    return {
        "underlying_type_id": underlying.get("type-id") if underlying is not None else None,
        "enumerators": enumerators,
    }


def build_fingerprint(xml_path, surface):
    """Extract from the corpus exactly what this side's headers declare."""
    root = ET.parse(xml_path).getroot()
    fp = {"classes": {}, "structs": {}, "enums": {}, "missing": []}
    for name in surface["vtable_classes"]:
        node = find_decl(root, "class-decl", name)
        if node is None:
            fp["missing"].append(("class", name))
            continue
        fp["classes"][name] = extract_class_fp(node)
    for name in surface["structs"] | surface["proctables"] | {WITNESS_STRUCT}:
        node = find_decl(root, "class-decl", name)
        if node is None:
            fp["missing"].append(("struct", name))
            continue
        fp["structs"][name] = extract_struct_fp(node)
    for name in surface["enums"]:
        node = find_decl(root, "enum-decl", name)
        if node is None:
            fp["missing"].append(("enum", name))
            continue
        fp["enums"][name] = extract_enum_fp(node)
    return fp


def assert_coverage_sane(fp, surface, label):
    """Every name this side's headers declare must be fully present in this
    side's corpus, and each class's emitted virtual count must equal the
    header's own `virtual` declaration count. This replaces any magic
    minimum: it adapts as the contract grows or shrinks, and it pinpoints
    WHICH class under-emitted. Failing here means the shim under-covers the
    contract (a new interface without an anchor, a header not #included, a
    compile without -femit-class-debug-always), which must never read as
    "clean" -- see contract_shim.cpp.
    """
    problems = []
    if fp["missing"]:
        problems.append("not emitted at all: %s" % fp["missing"])
    for name, header_count in surface["vtable_classes"].items():
        c = fp["classes"].get(name)
        if c is None:
            continue  # already in fp["missing"]
        if c["virtual_count"] != header_count:
            problems.append(
                "%s: header declares %d virtuals but the corpus emitted %d"
                % (name, header_count, c["virtual_count"]))
    if problems:
        raise SetupError(
            "%s: the compiled corpus does not cover what the headers "
            "declare:\n  %s\nLikely causes: a new interface class without "
            "an anchor call in contract_shim.cpp, a contract header the "
            "shim does not #include, or a compile without "
            "-femit-class-debug-always." % (label, "\n  ".join(problems)))


# ---------------------------------------------------------------------------
# Classification
# ---------------------------------------------------------------------------

def _appendable_struct(name):
    # ProcTable structs never cross the boundary as a layout -- the API side
    # builds its own from per-symbol dlsym -- so a member appended alongside
    # a new wrapper is a legitimate extension. The witness struct is this
    # tool's coverage roster: appending a member is how a new by-value type
    # *becomes* tracked. Every other tracked struct crosses by value, where
    # the old side owns the storage, so even append-at-end is unsafe there.
    return name.endswith("ProcTable") or name == WITNESS_STRUCT


def classify_fingerprints(base_fp, new_fp):
    """Return (verdict, items). verdict in unchanged/append/breaking.

    Both fingerprints were built against their own side's headers, so a key
    present on one side only means the *contract surface itself* changed --
    tracked on the new side only: a newly added interface (append); on the
    base side only: an interface removed from the contract (breaking).
    """
    items = []
    breaking = changed = False

    for name in base_fp["classes"]:
        if name not in new_fp["classes"]:
            items.append(("breaking", "class", name,
                         "removed from the contract headers"))
            breaking = changed = True
    for name in new_fp["classes"]:
        if name not in base_fp["classes"]:
            items.append(("append", "class", name, "newly tracked interface"))
            changed = True

    for name, base_c in base_fp["classes"].items():
        new_c = new_fp["classes"].get(name)
        if new_c is None:
            continue
        local_breaking = False
        base_slots, new_slots = base_c["slots"], new_c["slots"]
        for off, entry in base_slots.items():
            if off not in new_slots:
                items.append(("breaking", "vtable-slot",
                             "%s[%d] %s" % (name, off, entry[0]),
                             "slot removed"))
                local_breaking = True
            elif new_slots[off] != entry:
                items.append(("breaking", "vtable-slot",
                             "%s[%d]" % (name, off),
                             "%s -> %s" % (entry[0], new_slots[off][0])))
                local_breaking = True
        base_max = max(base_slots) if base_slots else -1
        for off, entry in new_slots.items():
            if off not in base_slots:
                if off <= base_max:
                    items.append(("breaking", "vtable-slot",
                                 "%s[%d] %s" % (name, off, entry[0]),
                                 "inserted before existing slots"))
                    local_breaking = True
                else:
                    items.append(("append", "vtable-slot",
                                 "%s[%d] %s" % (name, off, entry[0]),
                                 "new virtual appended"))
                    changed = True
        if base_c["dtor"] != new_c["dtor"]:
            items.append(("breaking", "destructor", name,
                         "%s -> %s" % (base_c["dtor"], new_c["dtor"])))
            local_breaking = True
        if (base_c["size_in_bits"] != new_c["size_in_bits"]
                and not local_breaking):
            items.append(("breaking", "class-size", name,
                         "%s -> %s bits with no matching per-slot change "
                         "(unmodelled)" %
                         (base_c["size_in_bits"], new_c["size_in_bits"])))
            local_breaking = True
        if local_breaking:
            breaking = changed = True

    for name in base_fp["structs"]:
        if name not in new_fp["structs"]:
            items.append(("breaking", "struct", name,
                         "removed from the contract headers"))
            breaking = changed = True
    for name in new_fp["structs"]:
        if name not in base_fp["structs"]:
            items.append(("append", "struct", name, "newly tracked"))
            changed = True

    for name, base_s in base_fp["structs"].items():
        new_s = new_fp["structs"].get(name)
        if new_s is None:
            continue
        local_breaking = False
        base_m, new_m = base_s["members"], new_s["members"]
        for mname, entry in base_m.items():
            if mname not in new_m:
                items.append(("breaking", "struct-member",
                             "%s.%s" % (name, mname), "member removed"))
                local_breaking = True
            elif new_m[mname] != entry:
                items.append(("breaking", "struct-member",
                             "%s.%s" % (name, mname),
                             "%s -> %s" % (entry, new_m[mname])))
                local_breaking = True
        appended = [m for m in new_m if m not in base_m]
        for mname in appended:
            if _appendable_struct(name):
                items.append(("append", "struct-member",
                             "%s.%s" % (name, mname), "member appended"))
                changed = True
            else:
                items.append(("breaking", "struct-member",
                             "%s.%s" % (name, mname),
                             "member appended to a by-value struct -- "
                             "unsafe, the old side owns the storage"))
                local_breaking = True
        if (base_s["size_in_bits"] != new_s["size_in_bits"]
                and not local_breaking and not appended):
            items.append(("breaking", "struct-size", name,
                         "%s -> %s bits, unmodelled" %
                         (base_s["size_in_bits"], new_s["size_in_bits"])))
            local_breaking = True
        if local_breaking:
            breaking = changed = True

    for name in base_fp["enums"]:
        if name not in new_fp["enums"]:
            items.append(("breaking", "enum", name,
                         "removed from the contract headers"))
            breaking = changed = True
    for name in new_fp["enums"]:
        if name not in base_fp["enums"]:
            items.append(("append", "enum", name, "newly tracked"))
            changed = True

    for name, base_e in base_fp["enums"].items():
        new_e = new_fp["enums"].get(name)
        if new_e is None:
            continue
        local_breaking = False
        base_vals = dict(base_e["enumerators"])
        new_vals = dict(new_e["enumerators"])
        for ename, val in base_vals.items():
            if ename not in new_vals:
                items.append(("breaking", "enumerator",
                             "%s::%s" % (name, ename), "removed"))
                local_breaking = True
            elif new_vals[ename] != val:
                items.append(("breaking", "enumerator",
                             "%s::%s" % (name, ename),
                             "%s -> %s" % (val, new_vals[ename])))
                local_breaking = True
        if base_e["underlying_type_id"] != new_e["underlying_type_id"]:
            items.append(("breaking", "enum-underlying-type", name,
                         "underlying type changed"))
            local_breaking = True
        new_only = [e for e in new_vals if e not in base_vals]
        if new_only and not local_breaking:
            for ename in new_only:
                items.append(("append", "enumerator",
                             "%s::%s" % (name, ename), "added"))
            changed = True
        if local_breaking:
            breaking = changed = True

    if breaking:
        return "breaking", items
    if changed:
        return "append", items
    return "unchanged", items


def i2_wrapper_delta(base_dir, new_dir):
    base_names = extract_wrapper_names(base_dir)
    new_names = extract_wrapper_names(new_dir)
    items = []
    for name in sorted(base_names - new_names):
        items.append(("breaking", "wrapper", name, "removed"))
    for name in sorted(new_names - base_names):
        items.append(("append", "wrapper", name, "added"))
    return items


# ---------------------------------------------------------------------------
# Repo-absolute invariants (I1, I3-count) -- not part of the base/new diff
# ---------------------------------------------------------------------------

def check_i1_current_repo():
    """Absolute invariant on the actual repo, independent of any base ref:
    every wrapper the contract headers declare must be dlsym'd by name
    somewhere, and every dlsym literal must correspond to a declared
    wrapper. This is what would have caught the CreateWithBuffer typo
    documented in LWEDelegateLoader.cpp before it shipped.
    """
    header_names = extract_wrapper_names(REPO_ROOT)
    loader_names = set()
    for relpath in LOADER_REL_PATHS:
        text = open(os.path.join(REPO_ROOT, relpath)).read()
        loader_names.update(DLSYM_RE.findall(text))
    only_headers = sorted(header_names - loader_names)
    only_loaders = sorted(loader_names - header_names)
    if only_headers or only_loaders:
        raise SetupError(
            "I1 violated: contract headers and dlsym literals in %s "
            "disagree.\n  declared but never dlsym'd: %s\n  dlsym'd but "
            "not declared: %s" %
            (", ".join(LOADER_REL_PATHS), only_headers, only_loaders))
    return header_names


def check_shim_covers_all_headers(tree_dir):
    contract_dir = os.path.join(tree_dir, CONTRACT_DIR)
    shim_text = open(os.path.join(tree_dir, SHIM_REL_PATH)).read()
    for fname in os.listdir(contract_dir):
        if not fname.endswith(".h"):
            continue
        if ('"public/contract/%s"' % fname) not in shim_text:
            raise SetupError(
                "contract_shim.cpp does not #include %s -- a new contract "
                "header was added without extending the shim's coverage" %
                fname)


def check_wrapper_assert_count_matches(tree_dir):
    header_names = extract_wrapper_names(tree_dir)
    shim_text = open(os.path.join(tree_dir, SHIM_REL_PATH)).read()
    # Exclude the macro's own #define line, which also matches
    # WRAPPER_CHECK_MACRO_RE -- only count actual invocations.
    invocation_lines = "\n".join(
        line for line in shim_text.splitlines()
        if not line.lstrip().startswith("#define"))
    macro_count = len(WRAPPER_CHECK_MACRO_RE.findall(invocation_lines))
    if macro_count != len(header_names):
        raise SetupError(
            "contract_shim.cpp has %d CONTRACT_ABI_CHECK_WRAPPER lines but "
            "the contract headers declare %d wrapper functions -- a "
            "wrapper was added or removed without updating the shim's I3 "
            "list" % (macro_count, len(header_names)))


# ---------------------------------------------------------------------------
# abidiff tripwire
# ---------------------------------------------------------------------------

def run_abidiff(base_xml, new_xml):
    result = subprocess.run(
        ["abidiff", "--fail-no-debug-info", base_xml, new_xml],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    report = result.stdout.decode("utf-8", "replace")
    # ABIDIFF_ERROR (1) or ABIDIFF_USAGE_ERROR (2) mean the tool itself
    # failed -- never conflate that with a real ABI change.
    if result.returncode & 1 or result.returncode & 2:
        raise SetupError("abidiff itself failed (exit %d):\n%s" %
                         (result.returncode, report))
    changed = bool(result.returncode & 4)  # ABIDIFF_ABI_CHANGE
    return changed, report


# ---------------------------------------------------------------------------
# The shared comparison pipeline.
#
# One function, used verbatim by both the production compare() and the
# --verify-checker fixtures. They MUST run the same code: an earlier draft
# had each assemble its own pipeline, and the fixture path quietly diverged
# from production (it skipped the abidiff cross-check, so "13/13 fixtures
# pass" certified a pipeline no real PR would ever run).
# ---------------------------------------------------------------------------

def compare_trees(base_dir, new_dir, base_label, new_label):
    """Compare two exported source trees. Returns (verdict, items, new_fp)."""
    check_shim_covers_all_headers(base_dir)
    check_shim_covers_all_headers(new_dir)
    check_wrapper_assert_count_matches(new_dir)

    base_surface = parse_contract_surface(base_dir)
    new_surface = parse_contract_surface(new_dir)

    base_so = compile_shim(base_dir)
    new_so = compile_shim(new_dir)

    base_xml = os.path.join(base_dir, "shim.abi.xml")
    new_xml = os.path.join(new_dir, "shim.abi.xml")
    dump_abi(base_so, base_xml)
    dump_abi(new_so, new_xml)

    base_fp = build_fingerprint(base_xml, base_surface)
    new_fp = build_fingerprint(new_xml, new_surface)
    assert_coverage_sane(base_fp, base_surface, base_label)
    assert_coverage_sane(new_fp, new_surface, new_label)

    xml_verdict, items = classify_fingerprints(base_fp, new_fp)

    # abidiff as a one-directional tripwire. If abidiff reports a change the
    # fingerprint completely missed, the fingerprint's model has a hole --
    # never trust either verdict then. The other direction is NOT a gap:
    # the fingerprint is deliberately more sensitive than abidiff (which
    # filters e.g. a const-qualifier change as "harmless" -- harmless for a
    # normal library, but a different mangled name and thus a different
    # dlsym/vtable identity across this boundary), and when the fingerprint
    # already reports a change, abidiff's silence adds no information.
    abidiff_changed, abidiff_report = run_abidiff(base_xml, new_xml)
    if abidiff_changed and xml_verdict == "unchanged":
        raise SetupError(
            "modelling gap: abidiff reported a change the structural "
            "fingerprint did not see -- do not trust either verdict.\n\n"
            "abidiff report:\n%s" % abidiff_report)

    items = items + i2_wrapper_delta(base_dir, new_dir)
    if any(kind == "breaking" for kind, *_ in items):
        verdict = "breaking"
    elif any(kind == "append" for kind, *_ in items):
        verdict = "append"
    else:
        verdict = "unchanged"
    return verdict, items, new_fp


# ---------------------------------------------------------------------------
# Waivers
# ---------------------------------------------------------------------------

def added_waiver_lines(ref):
    """ONLY lines added to the waiver file since ref. Deliberately not the
    file's current full text: old waiver lines merged by past PRs would then
    silently waive every future break that happens to reuse the same key
    (the keys are short -- "ResourceError[2]" -- so reuse is likely).
    """
    try:
        diff = git("diff", ref, "--", WAIVER_REL_PATH)
    except SetupError:
        diff = ""
    return "\n".join(line[1:] for line in diff.splitlines()
                     if line.startswith("+") and not line.startswith("+++"))


def unwaived_breaking_items(items, ref):
    text = added_waiver_lines(ref)
    return [(kind, category, key, detail)
            for kind, category, key, detail in items
            if kind == "breaking" and key not in text]


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def print_coverage(fp, label, verbose):
    print("Checked (%s):" % label)
    print("  Classes (virtual methods, matches each header's declaration count):")
    for name, c in sorted(fp["classes"].items()):
        print("    LWEDelegate::%-22s %3d" % (name, c["virtual_count"]))
        if verbose:
            for off in sorted(c["slots"]):
                print("        [%3d] %s" % (off, c["slots"][off][0]))
            if c["dtor"]:
                print("        [dtor] %s" % c["dtor"][0])
    proctables = sorted(n for n in fp["structs"] if n.endswith("ProcTable"))
    by_value = sorted(n for n in fp["structs"]
                      if not n.endswith("ProcTable") and n != WITNESS_STRUCT)
    print("  ProcTables (%d): %s" % (len(proctables), ", ".join(proctables)))
    print("  By-value structs (%d): %s" % (len(by_value), ", ".join(by_value)))
    print("  Enums (%d): %s" % (len(fp["enums"]), ", ".join(sorted(fp["enums"]))))
    print("  Witness constants (IdleModeCheckDefaultIntervalInMS)")


def print_verdict(verdict, items):
    if verdict == "unchanged":
        print("OK: delegate contract ABI unchanged.")
        return
    print("The delegate contract ABI %s:" %
         ("changed (BREAKING)" if verdict == "breaking" else "changed (append-only)"))
    for kind, category, key, detail in items:
        print("  [%s] %s :: %s -- %s" % (kind, category, key, detail))


# ---------------------------------------------------------------------------
# Main comparison
# ---------------------------------------------------------------------------

def base_predates_checker(base_ref):
    return SHIM_REL_PATH not in tracked_paths_at_ref(base_ref, SHIM_REL_PATH)


def bootstrap_smoke_test(base_ref, verbose):
    """base_ref has no contract_shim.cpp at all -- this is (or looks like)
    the PR introducing this checker, so there is no prior ABI to diff
    against. Rather than erroring out on every first-time PR, smoke-test
    that the working tree's own shim compiles with full coverage, and pass.
    A later PR that actually changes the contract will have a real base to
    compare against and go through the normal path.
    """
    print("NOTE: %s does not exist at %s -- nothing to compare against "
         "(this looks like the PR introducing this checker). "
         "Smoke-testing the working tree's shim only." %
         (SHIM_REL_PATH, base_ref))
    with tempfile.TemporaryDirectory(prefix="contract_abi.") as tmp:
        new_dir = os.path.join(tmp, "new")
        os.makedirs(new_dir)
        export_working_tree(new_dir)
        check_shim_covers_all_headers(new_dir)
        check_wrapper_assert_count_matches(new_dir)
        surface = parse_contract_surface(new_dir)
        new_so = compile_shim(new_dir)
        new_xml = os.path.join(tmp, "new.xml")
        dump_abi(new_so, new_xml)
        new_fp = build_fingerprint(new_xml, surface)
        assert_coverage_sane(new_fp, surface, "working tree")
    print_coverage(new_fp, "bootstrap (no prior baseline)", verbose)
    print("OK: bootstrap smoke test passed.")
    return 0


def compare(base_ref, verbose):
    check_abidw_version()
    require_tool("abidiff")
    require_tool("g++")

    if base_predates_checker(base_ref):
        return bootstrap_smoke_test(base_ref, verbose)

    check_i1_current_repo()

    with tempfile.TemporaryDirectory(prefix="contract_abi.") as tmp:
        base_dir = os.path.join(tmp, "base")
        new_dir = os.path.join(tmp, "new")
        os.makedirs(base_dir)
        os.makedirs(new_dir)
        export_ref_tree(base_ref, base_dir)
        export_working_tree(new_dir)
        verdict, items, new_fp = compare_trees(
            base_dir, new_dir, "base %s" % base_ref, "working tree")

    print_coverage(new_fp, "%s -> working tree" % base_ref, verbose)
    print_verdict(verdict, items)

    if verdict != "breaking":
        return 0

    unwaived = unwaived_breaking_items(items, base_ref)
    if not unwaived:
        print("\nAll breaking items are waived in %s." % WAIVER_REL_PATH)
        return 0

    print("\n%d breaking item(s) are not waived in %s:" %
         (len(unwaived), WAIVER_REL_PATH))
    for kind, category, key, detail in unwaived:
        print("  add a line naming: %s" % key)
    return 1


def resolve_base(args):
    ref = args.base or "origin/master"
    try:
        return resolve_ref(ref)
    except SetupError:
        if args.base:
            raise
        raise SetupError(
            "could not resolve %r as a base commit -- pass --base "
            "explicitly, or fetch it first (e.g. `git fetch origin "
            "master`)" % ref)


# ---------------------------------------------------------------------------
# --verify-checker: unit tests for this script and contract_shim.cpp, using
# synthetic fixtures run through compare_trees() -- the SAME pipeline a real
# PR goes through (minus I1, which reads the real repo's loader sources, and
# minus the waiver, which reads real git history). This does NOT check any
# real change -- see the module docstring.
# ---------------------------------------------------------------------------

def _mutate(path, old, new):
    text = open(path).read()
    if old not in text:
        raise SetupError("fixture setup failed: %r not found in %s" %
                         (old, path))
    open(path, "w").write(text.replace(old, new, 1))


def _append(path, text):
    with open(path, "a") as f:
        f.write(text)


FIXTURES = []


def fixture(name, expect):
    def decorator(fn):
        FIXTURES.append((name, expect, fn))
        return fn
    return decorator


@fixture("unchanged", "unchanged")
def _fixture_unchanged(tree_dir):
    pass


@fixture("virtual-reorder", "breaking")
def _fixture_reorder(tree_dir):
    path = os.path.join(tree_dir, CONTRACT_DIR, "ResourceErrorDelegate.h")
    _mutate(path,
           "    virtual int GetErrorCode() = 0;\n"
           "    virtual std::string GetDescription() = 0;\n",
           "    virtual std::string GetDescription() = 0;\n"
           "    virtual int GetErrorCode() = 0;\n")


@fixture("virtual-removed", "breaking")
def _fixture_removed(tree_dir):
    path = os.path.join(tree_dir, CONTRACT_DIR, "ResourceErrorDelegate.h")
    _mutate(path, "    virtual std::string GetUrl() = 0;\n", "")


@fixture("virtual-appended", "append")
def _fixture_appended(tree_dir):
    path = os.path.join(tree_dir, CONTRACT_DIR, "ResourceErrorDelegate.h")
    _mutate(path,
           "    virtual std::string GetUrl() = 0;\n",
           "    virtual std::string GetUrl() = 0;\n"
           "    virtual int GetNewThing() = 0;\n")


@fixture("return-type-only-change", "breaking")
def _fixture_return_type(tree_dir):
    path = os.path.join(tree_dir, CONTRACT_DIR, "ResourceErrorDelegate.h")
    _mutate(path, "    virtual int GetErrorCode() = 0;\n",
           "    virtual long GetErrorCode() = 0;\n")


@fixture("const-qualifier-change", "breaking")
def _fixture_const(tree_dir):
    # abidiff files this under "harmless" and reports no change; the
    # fingerprint sees the mangled name move. This fixture is what pins the
    # tripwire's one-directionality: were the tripwire to demand agreement
    # in both directions, this legitimate breaking verdict would turn into
    # an unwaivable exit-2 dead end.
    path = os.path.join(tree_dir, CONTRACT_DIR, "SettingsDelegate.h")
    _mutate(path, "virtual ::LWE::TTSMode GetTTSMode() const = 0;",
           "virtual ::LWE::TTSMode GetTTSMode() = 0;")


@fixture("by-value-struct-field-inserted", "breaking")
def _fixture_struct_insert(tree_dir):
    path = os.path.join(tree_dir, CONTRACT_DIR, "LWEWebContainerDelegate.h")
    _mutate(path,
           "    struct WebContainerArguments {\n        unsigned width;\n",
           "    struct WebContainerArguments {\n        unsigned extra;\n"
           "        unsigned width;\n")


@fixture("workerprocessstate-renumbered", "breaking")
def _fixture_enum_renumber(tree_dir):
    path = os.path.join(tree_dir, "inc", "LWEWorker.h")
    _mutate(path, "    None,\n    Terminated,\n",
           "    None = 1,\n    Terminated = 2,\n")


@fixture("ttsmode-value-changed", "breaking")
def _fixture_ttsmode(tree_dir):
    path = os.path.join(tree_dir, "inc", "PlatformIntegrationData.h")
    _mutate(path, "    Forced = 1,\n", "    Forced = 2,\n")


@fixture("wrapper-renamed-in-header-only", "breaking")
def _fixture_wrapper_rename_header(tree_dir):
    # Renames the wrapper in the header (and, to keep I3 satisfied and the
    # shim compiling, in the shim's matching CONTRACT_ABI_CHECK_WRAPPER
    # line too). The rename produces literally no DWARF difference (an
    # extern "C" declaration that is never defined or referenced beyond a
    # compile-time decltype emits nothing), so only I2 (source-text wrapper
    # name-set diff) can catch it -- this fixture proves the verdict reaches
    # "breaking" through I2 alone, with an ABI-identical corpus. (In a real
    # PR, I1 -- headers vs the loader's dlsym literals -- would also fire,
    # but I1 reads the real repo and is not part of compare_trees.)
    path = os.path.join(tree_dir, CONTRACT_DIR, "CookieManagerDelegate.h")
    _mutate(path, "LWEDelegate_CookieManager_Destroy",
           "LWEDelegate_CookieManager_DestroyRenamed")
    shim = os.path.join(tree_dir, SHIM_REL_PATH)
    _mutate(shim, "LWEDelegate_CookieManager_Destroy",
           "LWEDelegate_CookieManager_DestroyRenamed")


@fixture("proctable-signature-mismatch", "setup-error")
def _fixture_proctable_mismatch(tree_dir):
    # Changes ONLY the ProcTable field type, leaving the wrapper's own
    # declaration untouched -- a genuine inconsistency between the two,
    # which is exactly what the shim's I3 static_assert exists to catch at
    # compile time (compile_shim() raises SetupError).
    path = os.path.join(tree_dir, CONTRACT_DIR, "CookieManagerDelegate.h")
    _mutate(path, "uintptr_t (*GetInstance)();",
           "uintptr_t (*GetInstance)(int);")


@fixture("new-interface-class-added", "append")
def _fixture_new_interface(tree_dir):
    # A whole new vtable-bearing interface, anchored in the shim in the same
    # change -- the normal way the contract grows. Must classify as append
    # ("newly tracked"), not error out: an earlier draft hardcoded the
    # tracked-class set inside this script, which made every possible PR
    # ordering for this scenario a dead end.
    path = os.path.join(tree_dir, CONTRACT_DIR, "ResourceErrorDelegate.h")
    _mutate(path, "} // namespace LWEDelegate",
           "class EXPORT_UNMANAGED_API NewIface {\n"
           "public:\n"
           "    virtual void Poke() = 0;\n"
           "    virtual ~NewIface() = default;\n"
           "};\n\n"
           "} // namespace LWEDelegate")
    _append(os.path.join(tree_dir, SHIM_REL_PATH),
           '\nextern "C" __attribute__((visibility("default"))) void\n'
           "ContractAbiAnchorNewIface(LWEDelegate::NewIface* p)\n"
           "{\n    p->Poke();\n}\n")


def run_verify_checker():
    require_tool("g++")
    require_tool("abidw")
    require_tool("abidiff")
    check_abidw_version()

    failures = []
    for name, expect, mutate in FIXTURES:
        with tempfile.TemporaryDirectory(prefix="contract_abi_verify.") as tmp:
            base_dir = os.path.join(tmp, "base")
            new_dir = os.path.join(tmp, "new")
            os.makedirs(base_dir)
            os.makedirs(new_dir)
            export_working_tree(base_dir)
            export_working_tree(new_dir)
            try:
                mutate(new_dir)
                got, items, _ = compare_trees(base_dir, new_dir,
                                              "fixture-base", "fixture-new")
                if got != expect:
                    failures.append((name, expect, got, ""))
                else:
                    print("[PASS] %s" % name)
            except SetupError as e:
                if expect == "setup-error":
                    print("[PASS] %s (SetupError as expected)" % name)
                else:
                    failures.append((name, expect, "setup-error", str(e)))

    # Empty-corpus guard: compiling without -femit-class-debug-always emits
    # every contract class as declaration-only; the per-class header-vs-
    # corpus virtual-count check must catch that, never read it as "clean".
    with tempfile.TemporaryDirectory(prefix="contract_abi_verify.") as tmp:
        tree_dir = os.path.join(tmp, "new")
        os.makedirs(tree_dir)
        export_working_tree(tree_dir)
        so_path = os.path.join(tree_dir, "shim.so")
        cmd = (["g++"] + [f for f in COMPILE_FLAGS
                          if f != "-femit-class-debug-always"] +
              ["-I" + os.path.join(tree_dir, "src"),
               "-I" + os.path.join(tree_dir, "inc"),
               "-o", so_path, SHIM_REL_PATH])
        subprocess.run(cmd, cwd=tree_dir, stdout=subprocess.PIPE,
                      stderr=subprocess.STDOUT, check=True)
        xml_path = os.path.join(tmp, "shim.xml")
        dump_abi(so_path, xml_path)
        surface = parse_contract_surface(tree_dir)
        fp = build_fingerprint(xml_path, surface)
        try:
            assert_coverage_sane(fp, surface, "no -femit-class-debug-always")
            failures.append(("empty-corpus-guard", "SetupError", "no error",
                            ""))
        except SetupError:
            print("[PASS] empty-corpus-guard (caught as expected)")

    total = len(FIXTURES) + 1
    print()
    if failures:
        print("%d/%d fixtures failed:" % (len(failures), total))
        for name, expect, got, detail in failures:
            print("  %s: expected %s, got %s%s" %
                 (name, expect, got, (" (%s)" % detail) if detail else ""))
        return 1
    print("%d/%d fixtures passed." % (total, total))
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--base", help="git ref/commit to compare against "
                       "(default: origin/master)")
    parser.add_argument("--verbose", action="store_true",
                       help="print every vtable slot checked")
    parser.add_argument("--verify-checker", action="store_true",
                       help="run this script's own fixture tests instead "
                       "of checking a real change")
    args = parser.parse_args()

    try:
        if args.verify_checker:
            return run_verify_checker()
        base_ref = resolve_base(args)
        return compare(base_ref, args.verbose)
    except SetupError as e:
        print("check_contract_abi.py: %s" % e, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
