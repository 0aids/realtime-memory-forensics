import sys
import os

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
import numpy as np
import rmf_py as rmf
import time
import argparse
from multiprocessing import cpu_count

parser = argparse.ArgumentParser(description="Process a PID argument")
parser.add_argument("PID", type=int, help="Process ID (must be an integer)")
parser.add_argument("matchString", type=str, help="String to match")

args = parser.parse_args()
NUMTHREADS = cpu_count() - 1

pid = args.PID
MATCHSTRING = args.matchString
print(f"PID is: {pid}")
rmf.SetLogLevel(rmf.LogLevel.Error)
# Get memory maps - filter for larger regions directly
maps = rmf.getMapsFromPid(pid).filterHasPerms("r").filterActiveRegions(pid)


def processStuff(batcher):
    global time
    global rmf
    global MATCHSTRING
    start = time.perf_counter()
    snapshots = batcher.makeSnapshot(maps, pid)
    results = batcher.findString(snapshots, MATCHSTRING).flatten()
    print(f"Num snapshots: {len(snapshots)}")
    print(f"Num results: {len(results)}")
    if len(results):
        print(
            f"Raw bytes of first element: {np.array(rmf.MemorySnapshot(results[0], pid)).tobytes()}"
        )
    return time.perf_counter() - start


with rmf.Batcher(11) as batcher:
    print(f"time taken {processStuff(batcher):.5f}")
