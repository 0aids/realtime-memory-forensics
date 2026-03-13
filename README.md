# Realtime Memory Forensics (rmf)
A realtime program memory debugger / reverse engineering tool with
multi-threaded high-throughput scanning, interactive gui with python
bindings/scripting support, allowing reproducible analysis of key data flow
via node-graph visualisation, all done without analysing executed assembly.


# TODO
- [-] feat/testing: Setup an opencode agent that will create and run unit tests for all available functions and methods
      Partially implemented.
      Current tests are written by AI and tweaked by me.

- [x] feat: Incorporate MrpVec capabilities (assigning and adding nodes)
- [x] refac: fix build script for better incorporation of testing
- [x] feat: better testing and more coverage
- [x] refac: Remove PIDs (should be user managed, idk why i'm storing it in the mrps)
- [?] refac: Remove named values for now for simplification.
- [ ] feat: Add link details on hover
- [ ] feat: Add node details on hover
- [ ] feat: Incorporate auto linking using an inputted analyzer.
- [ ] feat: undo + redo
- [ ] feat: python interoperability
- [ ] feat: integrated python shell
- [ ] feat: integrated python scripting in gui and file saving
- [ ] feat: re-add named values?
- [ ] feat: graph serialisation?
- [ ] feat: lazy snapshots for reduced memory usage?
- [ ] done for now?

# memory graph structure/use
```c++
int main() {
    MemoryGraph graph;
    Analyzer analyzer(6 /*threads*/);
    pid_t pid = /*pid here*/;
    int32_t targetValue = 0x98989898;

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath);
    auto readableRegions = regions.FilterHasPerms("r").FilterActiveRegions(pid);

    auto snaps = analyzer.Execute(MemorySnapshot::Make, readableRegions);
    MemoryRegionProperties result;
    {
    	result.clear();
        auto intermediate = analyzer.Execute(findNumeralExact<int32_t>,
                                       snaps, targetValue) |
            std::views::join;
        std::move(intermediate.begin(), intermediate.end(),
                  std::back_inserter(result));
    }
}
```


# Running tests
Make sure that you have memory limits setup otherwise it will crash your computer on failing
tests sometimes.
```bash
cmake -S . -B build -Dbuild_tests=ON && cmake --build build -j 12 && (ulimit -m 1000000 && ulimit -v 1000000 && cd build && ctest)
```


# Example script
```python
# file: globalVars.py
# Global Variables
# These are a script that is accessible in any script, and are imported before the function
# via "from globalVars import *"
pointerValueName = "Value to my pointer"
otherName = "Other name"

# file: script1.py # Currently cannot have names, but will have that functionality later.
# Do not modify these values here in the arguments
def getPointersToValue(memoryGraph, analyzer, maps):
    targetValue = -10685103945
    transaction = memoryGraph.StartCreateMemoryRegionTransaction()
    # Returns the class with the ID filled out for us.
    # Remember this name, that's how we will access the value later.
    transaction.name = pointerValueName
    transaction.comment = "Used for something"
    readableMaps = maps.FilterByHasPerms("r")

    snaps = analyzer.Execute(MemorySnapshot, readableMaps)

    results = CompressNestedMrpVec(analyzer.Execute(findNumericExact_i64, targetValue))

    # do some more stuff with the results to get your desired values.
    mrp = results...desiredMrp

    # Valuable information about the sizes of your desired thing
    transaction.mrp = mrp
    # Maybe you want to increase the size
    transaction.mrp.relativeRegionSize += 0x10 # add by 16 bytes
    transaction.mrp.relativeRegionSize += types.i8.size # add by 1 byte
    transaction.namedValues.add(
        name="randomNamedValue",
        offset=0x4,
        type=rmf_py.types.i32,
    )

    memoryGraph.ProcessTransaction(transaction)

# file: script2.py
def updateThatRegion(memoryGraph, analyzer, maps):
    transaction = memoryGraph.StartUpdateMemoryRegionTransaction(name=pointerValueName)

    # update your values in some way you want.
    # Can even change the name and what not.
    # Just remember that you did that.
    # Whatever you do, do not remove or change the id.

    memoryGraph.ProcessTransaction(transaction)

# file: script3.py
def findLinks(memoryGraph, analyzer, maps):
    # In this example we have a couple memory regions already made, named "a", "b", and "c"
    # Give us the next usable ID. You can create loops to do this etc.
    t1 = memoryGraph.StartCreateMemoryLinkTransaction()
    t2 = memoryGraph.StartCreateMemoryLinkTransaction()
    t3 = memoryGraph.StartCreateMemoryLinkTransaction()
```
