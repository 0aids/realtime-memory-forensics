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
- [x] feat: python interoperability / bindings
- [x] feat: integrated python shell
- [x] feat: multi threading in python
- [ ] feat: implement memory graph algorithms.
- [ ] feat: integrated python scripting in gui and file saving
- [ ] feat: Add link details on hover
- [ ] feat: Add node details on hover
- [ ] feat: Incorporate auto linking using an inputted ANALYZER.
- [ ] feat: undo + redo
- [ ] feat: re-add named values?
- [ ] feat: graph serialisation?
- [ ] feat: lazy snapshots for reduced memory usage?
           This honestly might have to be done now because
           I'm struggling to get the binded analyzer working at a sufficient speed.
           The only problem is delays between snapshots, which wouldnt work
           as easily.

- [ ] done for now?

# memory graph structure/use
```c++
int main() {
    MemoryGraph graph;
    Analyzer ANALYZER(6 /*threads*/);
    pid_t PID = /*PID here*/;
    int32_t targetValue = 0x98989898;

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto REGIONS         = ParseMaps(mapsPath);
    auto readableRegions = REGIONS.FilterHasPerms("r").FilterActiveRegions(PID);

    auto snaps = ANALYZER.Execute(MemorySnapshot::Make, readableRegions);
    MemoryRegionPropertiesVec result;
    {
    	result.clear();
        result = ANALYZER.Execute(findNumeralExact<int32_t>,
                                       snaps, targetValue).flatten();
    }
}
```


# Running tests
Make sure that you have memory limits setup otherwise it will crash your computer on failing
tests sometimes.
```bash
cmake -S . -B build -Dbuild_tests=ON && cmake --build build -j 12 && (ulimit -m 1000000 && ulimit -v 1000000 && cd build && ctest)
```


# Example script for gui
Basic script
```python
import rmf_py as rmf # preinjected / run within the embedded interpreter before everything.
import rmf_gui_py as rmfg # preinjected / run within the embedded interpreter before everything.
PID = # pre injected / run within the embedded interpreter before everything.
ANALYZER = rmf.Analyzer(6) # pre injected / run within the embedded interpreter before everything.
REGIONS = rmf.parseMaps(PID) # pre injected / run within the embedded interpreter before everything.

# Filters for regions actively in memory, and then breaks them into chunks with sizes 0x1000
chunkedRegions = REGIONS.filterActiveRegions().breakIntoChunks(0x10000, overlap=0)
snapshots = ANALYZER(rmf.makeSnapshot, chunkedRegions)

# returns a list of lists of memory REGIONS. which is then flattened
# Not really sure about this api.
results = ANALYZER.execute(rmf.findString, snapshots, "hello world!").flatten()

results.batchModify(sizeDiff=+0x100, addrDiff=-0x50)

# Dump the results into a graph
mg = rmf.MemoryGraph()
mg.pushNodes(results)

# Create links between nodes that exist in the graph.
# Does this naively by analyzing 8 byte chunks within each
# region and sees if it links into or inside another region
# already in the graph.
mg.findLinksStrict(ANALYZER)

rmfg.pushMemoryGraph(mg, policy="combine")
```

Finding changing regions (as an extension on what happened previously
```python
import rmf_py as rmf # preinjected / run within the embedded interpreter before everything.
import rmf_gui_py as rmfg # preinjected / run within the embedded interpreter before everything.
PID = # pre injected / run within the embedded interpreter before everything.
ANALYZER = rmf.Analyzer(6) # pre injected / run within the embedded interpreter before everything.
REGIONS = rmf.parseMaps(PID) # pre injected / run within the embedded interpreter before everything.

chunkedRegions = REGIONS.filterActiveRegions().filterHasPerms("rw").breakIntoChunks(0x10000, overlap=0)

from copy import deepcopy
results = deepcopy(chunkedRegions)

from time import sleep

while len(results) > 10:
    snaps1 = ANALYZER.execute(rmf.makeSnapshot, results)
    sleep(1) # wait 1s
    snaps2 = ANALYZER.execute(rmf.makeSnapshot, results)

    results = ANALYZER.execute(rmf.findChangedRegions, snaps1, snaps2, 32) # using 32 byte chunks.

# Only modify relative parameters obviously
results.batchModify(sizeDiff = +10, addrDiff = -10)
# or we can set them to be batchStructified?

mg = rmfg.pullMemoryGraph()
mg.pushNodes(results)

# Relaxed, so creates nodes which point to existing nodes.
# internally will update, but also will return a list of new mrps generated
mg.findLinksRelaxed(ANALYZER, REGIONS)

# Or something similar
# Prune links that point to memory addresses lower than 0x1000000
mg.pruneLinks(lambda link: link.targetAddr < 0x1000000)
# alternatively
mg.keepLinks(lambda link: link.targetAddr >= 0x1000000)

# Or we can prune or keep nodes
discardedNodes = mg.pruneNodes(lambda node: node.mrp.relativeRegionSize < 0x1000)

# Advanced? psuedo creating structs and assigning a node that struct
# Allows for easier accessing and interpreting of data related to that node
# name is not needed.
dataStruct = rmf.Struct.parseC("""
struct {
    int32_t testInteger;
    char* string;
    int16_t int16array[10];
};
""")

# Get the desired node (returns a copy which we have to assign back.)
nodeId, node = mg.nodes.filter(lambda node: ...)[0]

# Assign a struct. CurrentValueName is to figure out the offset
# incase we only captured a certain value within the struct
# also would modify the node's mrp to now fit this struct. also returns
# itself so it can be used within a map + lambda
node.assignStruct(dataStruct, currentValueName="testInteger")
mg.updateNode(nodeId, node)

# Or we can do this on a set of nodes via python's map
nodeIds, nodes = mg.nodes.filter(lambda node: ...)
nodes.batchAssignStructs(dataStruct, currentValueName="testInteger")
mg.batchUpdateNodes(nodeIds, nodes)


# and so on

# and then when happy with the results we can push it again.
rmfg.pushMemoryGraph(mg, policy="override")
# or (will prune duplicates)
rmfg.pushMemoryGraph(mg, policy="combine")
# or (will not prune duplicates)
rmfg.pushMemoryGraph(mg, policy="add")
# EXTRA: rmfg is just memorygraph, and all memory graphs allow pushing and pulling
# with policies. This would allow and separation of graphs if need be.
```
