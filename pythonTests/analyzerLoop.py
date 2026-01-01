from rmf_py import *
import numpy as np

numThreads = int(input("Num threads: " ))
pid = int(input("Give me the pid: "))
location = "/proc/" + str(pid) + "/maps"
logLevel = input("Input loglevel (or empty to continue)(e, w, i, v, d): ")
match logLevel:
    case "e":
        SetLogLevel(LogLevel.Error);
    case "w":
        SetLogLevel(LogLevel.Warning);
    case "i":
        SetLogLevel(LogLevel.Info);
    case "v":
        SetLogLevel(LogLevel.Verbose);
    case "d":
        SetLogLevel(LogLevel.Debug);
    case _:
        print("No option or invalid, continuing");


chunkSize = 0x10000

regions = ParseMaps(location, pid).FilterPerms("rwp").BreakIntoChunks(chunkSize, 0)
lastRegions = []

anal = Analyzer(numThreads)

while True:
    try:
        a = ""
        a = input("Enter (1 for changing. 2 for unchanging. 3 for behind bear, 4 to undo last: ");
        print("keyboard interrupt to cancel")
        match a:
            case "1":
                print("Performing change detection")
                print("Snapshotting 1!")
                snap1 = anal.execute(MemorySnapshot, regions)
                a = input("Enter to snapshot 2: ");
                print("Snapshotting 2!")
                snap2 = anal.execute(MemorySnapshot, regions)
                print("Finding changed regions")
                lastRegions = regions
                regions = CompressNestedMrpVec(anal.execute(findChangedRegions, snap1, snap2, 4))

            case "2":
                print("Performing unchanged detection")
                print("Performin change detection")
                print("Snapshotting 1!")
                snap1 = anal.execute(MemorySnapshot, regions)
                a = input("Enter to snapshot 2: ");
                print("Snapshotting 2!")
                snap2 = anal.execute(MemorySnapshot, regions)
                print("Finding unchanged regions")
                lastRegions = regions
                regions = CompressNestedMrpVec(anal.execute(findUnchangedRegions, snap1, snap2, 4))

            case "3":
                print("Performing numeric determination")
                print("Snapshotting 1!")
                snap1 = anal.execute(MemorySnapshot, regions)
                print("finding float!")
                lastRegions = regions
                regions = CompressNestedMrpVec(anal.execute(findNumeralWithinRange_f32, snap1, -295.0, -285.0))

            case "4":
                print("Undoing!");
                regions = lastRegions;

        print(f"There are {len(regions)} regions left")
    except KeyboardInterrupt:
        print("Exiting loop!")
        break;

print("do whatever now. ")
print(f"Current values available: {globals().keys()}")
