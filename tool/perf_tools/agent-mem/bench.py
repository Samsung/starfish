#!/usr/bin/env python3
# Measures engine memory (and optionally layout geometry) per page through the
# LWE CLI, so builds/backends can be compared on the same page set.
import argparse
import json
import os
import subprocess
import sys
import tempfile
import time

GEOM_JS = """(function(){var r=[];var es=document.querySelectorAll('body *');
for(var i=0;i<es.length&&i<%d;i++){var b=es[i].getBoundingClientRect();
r.push([es[i].tagName,Math.round(b.x),Math.round(b.y),Math.round(b.width),Math.round(b.height)]);}
return JSON.stringify(r);})()"""


def cli(args, *cmd, timeout=60):
    p = subprocess.run([args.cli] + list(cmd), capture_output=True, text=True,
                       timeout=timeout, env=args.env)
    if p.returncode != 0:
        raise RuntimeError("%s failed: %s" % (cmd[0], p.stderr.strip()))
    return p.stdout.strip()


def eval_js(args, js):
    return json.loads(cli(args, "eval", js))


def engine_pid(args):
    engine = os.path.join(os.path.dirname(os.path.abspath(args.cli)),
                          "lightweight-web-engine")
    home = ("HOME=" + args.env["HOME"]).encode()
    out = subprocess.run(["pgrep", "-f", "^" + engine + " "],
                         capture_output=True, text=True).stdout.split()
    # Only this run's session engine carries the private HOME.
    for pid in out:
        try:
            with open("/proc/%s/environ" % pid, "rb") as f:
                if home in f.read().split(b"\0"):
                    return int(pid)
        except OSError:
            pass
    raise RuntimeError("engine process not found")


def memory(pid):
    m = {}
    with open("/proc/%d/smaps_rollup" % pid) as f:
        for line in f:
            k, _, v = line.partition(":")
            if k in ("Rss", "Pss", "Pss_Anon"):
                m[k] = int(v.split()[0])
    with open("/proc/%d/status" % pid) as f:
        for line in f:
            if line.startswith("VmHWM:"):
                m["VmHWM"] = int(line.split()[1])
    return m


def measure(args, url):
    cli(args, "open", url, timeout=120)
    deadline = time.time() + args.load_timeout
    while eval_js(args, "document.readyState") != "complete":
        if time.time() > deadline:
            break
        time.sleep(0.5)
    time.sleep(args.settle)
    result = {"url": url}
    result.update(memory(engine_pid(args)))
    if args.geom:
        result["geom"] = json.loads(eval_js(args, GEOM_JS % args.geom))
    cli(args, "close")
    return result


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--cli", required=True, help="lightweight-web-engine-cli path")
    ap.add_argument("--settle", type=float, default=3.0, help="seconds after load")
    ap.add_argument("--load-timeout", type=float, default=30.0)
    ap.add_argument("--geom", type=int, default=0,
                    help="also record bounding boxes of the first N elements")
    ap.add_argument("--out", help="write JSON results here")
    ap.add_argument("urls", nargs="+")
    args = ap.parse_args()
    # A private HOME isolates the CLI session from any other running session.
    args.env = dict(os.environ, HOME=tempfile.mkdtemp(prefix="agent-mem-"))
    results = []
    print("%-60s %9s %9s %9s %9s" % ("url (kB)", "Rss", "Pss", "Pss_Anon", "VmHWM"))
    for url in args.urls:
        r = measure(args, url)
        results.append(r)
        print("%-60s %9d %9d %9d %9d" % (url[-60:], r["Rss"], r["Pss"],
                                         r["Pss_Anon"], r["VmHWM"]))
    if args.out:
        with open(args.out, "w") as f:
            json.dump(results, f, indent=1)


if __name__ == "__main__":
    sys.exit(main())
