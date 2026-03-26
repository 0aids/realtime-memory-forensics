# Realtime Memory Forensics (rmf)
A realtime program memory debugger / reverse engineering tool with
multi-threaded high-throughput scanning, interactive gui with python
bindings/scripting support, allowing reproducible analysis of key data flow
via node-graph visualisation, all done without analysing executed assembly.


# TODO
- [x] feat: Incorporate MrpVec capabilities (assigning and adding nodes)
- [x] refac: fix build script for better incorporation of testing
- [x] feat: better testing and more coverage
- [x] refac: Remove PIDs (should be user managed, idk why i'm storing it in the mrps)
- [?] refac: Remove named values for now for simplification.
- [x] feat: python interoperability / bindings
- [x] feat: integrated python shell
- [x] feat: multi threading in python
    - [x] feat: faster multithreading via Batcher.
- [-] feat: implement memory graph algorithms.
    - [x] Get structs working.
    - [ ] MemoryGraphData wrapper MemoryGraph.
    - [x] Basic prototype of MemoryGraphData without structs
- [ ] feat: integrated python scripting in gui and file saving
- [ ] feat: Add link details on hover
- [ ] feat: Add node details on hover
- [ ] feat: Incorporate auto linking using an inputted ANALYZER.
- [ ] feat: undo + redo
- [ ] feat: graph serialisation?
- [x] fix: Find that stupid race condition
- [ ] feat: lazy snapshots for reduced memory usage?
           The only problem is delays between snapshots, which wouldnt work
           as easily. The only way to get this to work is to have
           some sort of global vector storing times at which each snapshot was
           "Created"? Or just make this sort of snapshot not compatible with
           searching for changed memory?

- [ ] done for now?


# Running tests
Make sure that you have memory limits setup otherwise it will crash your computer on failing
tests sometimes.
```bash
cmake -S . -B build -Dbuild_tests=ON && cmake --build build -j 12 && (ulimit -m 1000000 && ulimit -v 1000000 && cd build && ctest)
```

# planned memory graph structure/use
```cpp
using namespace rmf;
using namespace rmf::types;
using namespace rmf::utils;
using namespace rmf::op;
int main() {
    const pid_t PID = /*PID here*/;
    Analyzer analyzer(6 /*threads*/);
    MemoryGraph graph(PID, analyzer, defaultStructRegistry);
    int32_t targetValue = 0x98989898;
    auto REGIONS         = getMapsFromPid(PID);
    auto readableRegions = REGIONS.FilterHasPerms("r").FilterActiveRegions(PID);

    auto snaps = analyzer.Execute(MemorySnapshot::Make, readableRegions);
    MemoryRegionPropertiesVec result;
    {
    	result.clear();
        result = analyzer.Execute(findNumeralExact<int32_t>,
                                       snaps, targetValue).flatten();
    }
    auto newResult = analyzer.Execute(RestructureMrp, MrpRestructure{
    	.offset = -0xf;
    	.sizeDelta = +0xf;
    });
    // And whatever stuff to get the final desired result

    MemoryRegionPropertiesVec desiredResult = ...;

    // Alternatively we can make use of known structs to generate desired results.
    // These known structs must have pointer members at defined offsets.
    // This must be done to a "StructRegistry" so structs can refer to eachother.
    StructRegistry sr;
    sr.registerr("IntLinkedList") // Automatically calculates size.
    	.field("next", "IntLinkedList*")
    	.field("data", "int32_t")
    	.end()
    sr.registerr("IntVector")
    	.field("numElements", "size_t")
    	.field("data", "int32_t*") // has a list of preregistered fundamental datatypes.
    	.end();

    sr.registerr("flatString1") // Automatically calculates size.
		.field("string", "char[10]") // has a list of preregistered fundamental datatypes.
        .end();
    // Or this
    sr.registerTemplated<StructHere>("CustomNameOfStruct");
    graph.structRegistry.combine(sr);

	// Adds nodes that assumes points to the top of an IntVector.
    auto keys = graph.addNodes(desiredResult, "IntVector");
    // Alternatively we can say it was part of the data field
    // auto keys = graph.addNodes(desiredResult, "IntVector", "data");

    // We can then find sources or anything that references it.
    // By default these nodes will use a voidpointer struct
    auto [newSourceKeys, newLinkKeys] = graph.findSources(keys);
    println("{}", graph.structstructIdtoString(graph[newSourceKeys[0]].structId));
    // prints "voidpointer"

    // We can also prune the graph for stuff that's now out of date.
    graph.pruneStale();
    // Equivalent to
    // auto [invalidLinks, invalidNodes] = graph.getStale();
    // graph.removeLinks(invalidLinks);
    // graph.removeNodes(invalidNodes);

    // Now do whatever you want for visualising the graph

    // Unimplemented.
    // graphToSvg(graph.getData());
}
```


# Planned Example script for gui
Basic script
```python
import rmf_py as rmf # preinjected / run within the embedded interpreter before everything.
import rmf_gui_py as rmfg # preinjected / run within the embedded interpreter before everything.
PID = # pre injected / run within the embedded interpreter before everything.
BATCHER = rmf.Batcher(6) # pre injected / run within the embedded interpreter before everything.
REGIONS = rmf.parseMaps(PID) # pre injected / run within the embedded interpreter before everything.

# Filters for regions actively in memory. This breaks the regions into chunks
# of PAGESIZE.
chunkedRegions = REGIONS.filterActiveRegions()
snapshots = BATCHER(rmf.makeSnapshot, chunkedRegions)

results = BATCHER.findString(snapshots, "hello world!").flatten()

# Dump the results into a graph
mg = rmf.MemoryGraph(BATCHER, PID, structRegistry=rmf.defaultStructRegistry)
# the struct registry has defaults containing one for void* pointers.
# This is called "unknownPointer"
# If a node is not registered with a struct it will assume that it points to
# something at it's origin (if pointer aligned).
mg.structRegistry.register(
	Struct("helloWorldStr")
    	.field("chars", "char[13]")
)

mg.structRegistry.register(
	Struct("charstar")
    	.field("c", "char*")
)

# Coerces the MRP to fit the given struct.
keysToNodes = mg.addNodes(results, structName="helloWorldStr", field="chars")
# Fields is optional, but if not specified it will choose the top of the struct.

# Searches for char* pointing to our helloWorldStr
# Filter means it can only search for those structs.
newSourcesNodeKeys = mg.findSources(keysToNodes, structName="charstar", filter="helloWorldStr")
mg.pruneStale()
# equivalent to:
# deadNodeKeys, deadLinkKeys = mg.getStale()
# mg.removeNodes(deadNodeKeys)
# mg.removeLinks(deadLinkKeys)
# which removes nodes and links that have changed what they point to and no longer valid.
# if you only want to remove links that are now stale
mg.pruneStaleLinks()

# Push the memorygraph into the gui for viewing.
rmfg.pushMemoryGraph(mg, policy="combine")
# Links will not be made between the graphs.
```

Finding changing regions (as an extension on what happened previously
```python
import rmf_py as rmf # preinjected / run within the embedded interpreter before everything.
import rmf_gui_py as rmfg # preinjected / run within the embedded interpreter before everything.
PID = # pre injected / run within the embedded interpreter before everything.
BATCHER = rmf.Batcher(6) # pre injected / run within the embedded interpreter before everything.
REGIONS = rmf.parseMaps(PID) # pre injected / run within the embedded interpreter before everything.

mg = rmf.MemoryGraph(BATCHER, PID)
chunkedRegions = REGIONS.filterActiveRegions().filterHasPerms("rw")

from copy import deepcopy
results = deepcopy(chunkedRegions)

from time import sleep

while len(results) > 10:
    snaps1 = BATCHER.makeSnapshot(results)
    sleep(1)
    snaps2 = BATCHER.makeSnapshot(results)

    results = BATCHER.findChangedRegions(snaps1, snaps2, 32) # using 32 byte chunks.

results = BATCHER.mrpRestructure(sizeDiff = +10, addrDiff = -10)
# Advanced? psuedo creating structs and assigning a node that struct
# Allows for easier accessing and interpreting of data related to that node
# name is not needed.
dataStruct = rmf.Struct.parseC("""
struct testStruct {
    int32_t testInteger;
    char* string;
    int16_t int16array[10];
};
""")
# this say that the root of results was testInteger
results = nodify(results, "testStruct", "testInteger")
mg.pushNodes(results)

# find from testStruct to other existing nodes
mg.findLinks(source="testStruct", target="helloWorldStr")
```
