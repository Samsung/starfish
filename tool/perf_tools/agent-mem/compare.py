#!/usr/bin/env python3
# Compares two bench.py --out files: memory deltas and geometry mismatches.
import json
import sys


def main():
    a, b = (json.load(open(p)) for p in sys.argv[1:3])
    tol = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    print("%-40s %9s %9s %6s %9s %9s %6s %8s" % ("url", "Pss A", "Pss B", "B/A",
                                                 "Anon A", "Anon B", "B/A", "geom"))
    other = {r["url"]: r for r in b}
    for ra in a:
        rb = other.get(ra["url"])
        if rb is None:
            print("%-40s missing in B" % ra["url"][-40:])
            continue
        ga, gb = ra.get("geom"), rb.get("geom")
        geom = "-"
        if ga is not None and gb is not None:
            bad = sum(1 for ea, eb in zip(ga, gb)
                      if ea[0] != eb[0] or any(abs(x - y) > tol for x, y in zip(ea[1:], eb[1:])))
            bad += abs(len(ga) - len(gb))
            geom = "%d/%d" % (bad, max(len(ga), len(gb)))
        print("%-40s %9d %9d %6.2f %9d %9d %6.2f %8s" % (
            ra["url"][-40:], ra["Pss"], rb["Pss"], rb["Pss"] / ra["Pss"],
            ra["Pss_Anon"], rb["Pss_Anon"], rb["Pss_Anon"] / ra["Pss_Anon"], geom))


if __name__ == "__main__":
    sys.exit(main())
