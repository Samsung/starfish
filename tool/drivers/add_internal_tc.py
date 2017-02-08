#!/usr/bin/env python
INTERAL_TC_LIST = "tool/reftest/internal_unsorted.res"

if __name__ == "__main__":
    import os
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("tc")
    args = parser.parse_args()

    if not os.path.isfile(args.tc):
        import sys
        print "Wrong argument [tc] : " + args.tc + " is not a file"
        sys.exit()

    import subprocess
    starfish_command = ["./StarFish", args.tc, "--hide-window", "--width=800", "--height=600"]
    try:
        FNULL = open(os.devnull, "w")
        from datetime import datetime
        start_time = datetime.now()
        print "Running..."
        subprocess.call(starfish_command, stdout=FNULL, stderr=subprocess.STDOUT)
        elapsed_time = int((datetime.now() - start_time).total_seconds() * 1000)
        print "Elaped time : " + str(elapsed_time) + " ms"
        with open(INTERAL_TC_LIST, "a") as to_list:
            to_list.write(str(elapsed_time) + ":" + args.tc)
        print "Add internal TC successfully"
    except IOError:
        print "ERROR : Crash - " + args.tc
