from rmf_py import *

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

anal = Analyzer(numThreads)

a = ""
while not a:
    a = ""
    a = input("Go to noob shop window. Input something to quit (enter to cont.): ");
    if a: break;
    snap1 = anal.execute(MemorySnapshot, regions)
    regions = CompressNestedMrpVec(anal.execute(findNumeralWithinRange_f32, snap1, 61.1, 61.2))
    print(f"There are {len(regions)} regions left")
    print("Peforming changing region loop")
    a = input("Input something to quit (enter to cont.): ");
    if a: break;
    snap1 = anal.execute(MemorySnapshot, regions)
    a = input("Input something to quit (enter to cont.): ");
    if a: break;
    snap2 = anal.execute(MemorySnapshot, regions)
    print("Finding changed regions")
    regions = CompressNestedMrpVec(anal.execute(findChangedRegions, snap1, snap2, 4))
    print(f"There are {len(regions)} regions left")
    print("Performing non-changing loop")
    a = input("Input something to quit (enter to cont.): ");
    if a: break;
    print()
    snap1 = anal.execute(MemorySnapshot, regions)
    a = input("Input something to quit (enter to cont.): ");
    if a: break;
    snap2 = anal.execute(MemorySnapshot, regions)
    print("Finding unchanged regions")
    regions = CompressNestedMrpVec(anal.execute(findUnchangedRegions, snap1, snap2, 4))
    print(f"There are {len(regions)} regions left")
    a = input("Input something to quit (enter to cont.): ");
    if a: break;

print("do whatever now. ")
print(f"Current values available: {globals().keys()}")
