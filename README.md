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
    - [x] feat: faster multithreading via Batcher.
- [-] feat: implement memory graph algorithms.
- [ ] feat: integrated python scripting in gui and file saving
- [ ] feat: Add link details on hover
- [ ] feat: Add node details on hover
- [ ] feat: Incorporate auto linking using an inputted ANALYZER.
- [ ] feat: undo + redo
- [ ] feat: graph serialisation?
- [ ] fix: Find that stupid race condition
- [ ] feat: lazy snapshots for reduced memory usage?
           The only problem is delays between snapshots, which wouldnt work
           as easily. The only way to get this to work is to have
           some sort of global vector storing times at which each snapshot was
           "Created"? Or just make this sort of snapshot not compatible with
           searching for changed memory?

- [ ] done for now?

# memory graph structure/use
```cpp
int main() {
    const pid_t PID = /*PID here*/;
    Analyzer analyzer(6 /*threads*/);
    MemoryGraph graph(PID, analyzer);
    // you can substitute out the analyzer as well. via graph.setAnalyzer(...)
    // Don't have to worry about copies here as well, analyzer is trivially copyable (sptr)
    int32_t targetValue = 0x98989898;

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto REGIONS         = ParseMaps(mapsPath);
    auto readableRegions = REGIONS.FilterHasPerms("r").FilterActiveRegions(PID);

    auto snaps = analyzer.Execute(MemorySnapshot::Make, readableRegions);
    MemoryRegionPropertiesVec result;
    {
    	result.clear();
        result = analyzer.Execute(findNumeralExact<int32_t>,
                                       snaps, targetValue).flatten();
    }
    auto newResult = result.batchResize(ResizeInstruction{
    	.offset = -0xf;
    	.sizeDelta = +0xf;
    });
    // Or inplace via result.batchResizeInPlace
    // And whatever stuff to get the final desired result

    MemoryRegionPropertiesVec desiredResult = ...;

    // Dump this into a memory graph.

    graph.pushMrpVec(desiredResult);

    // From here we can "recursively" bfs to find sources that point to our desired results,
    // within the range given by ResizeInstruction. Can find and detect cycles.
    graph.findSourcesRecursiveLenient(PID, analyzer, ResizeInstruction{
    	.offset = -0xf;
    	.sizeDelta = 0xf;
    }, 10/*max depth*/, FindSourcePolicy::MinimumSize);
    // FindSourcePolicy::MinimumSize modifies the final mrp to be
    // minimum size that still contains the pointed to region.
    // FindSourcePolicy::MaximumSize modifies the final mrp to be the offsetted version.

    // Alternatively we can make use of known structs to generate desired results.
    // These known structs must have pointer members at defined offsets.
    // This must be done to a "StructRegistry" so structs can refer to eachother.
    StructRegistry sr;
    sr.register( // Automatically calculates size.
    	{
        	.name = "IntLinkedList",
        	.fields = {
            	{
            		.name = "next",
            		.type = "IntLinkedList*", // can parse pointers
            		.offset = 0, // For now, only manual specification of offsets.
        		},
        		{
        			.name = "data",
        			.type = "int32_t", // has a list of preregistered fundamental datatypes.
        			.offset = sizeof(void*),
        		},
        	},
    	}
    );
    sr.register( // Automatically calculates size.
    	{
        	.name = "IntVector",
        	.fields = {
            	{
            		.name = "numElements",
            		.type = "size_t", // can parse pointers
            		.offset = 0, // For now, only manual specification of offsets.
        		},
        		{
        			.name = "data",
        			.type = "int32_t*", // has a list of preregistered fundamental datatypes.
        			.offset = sizeof(size_t),
        		},
        	},
    	}
    );
    sr.register( // Automatically calculates size.
    	{
        	.name = "flatString1",
        	.fields = {
        		{
        			.name = "string",
        			.type = "char[10]", // has a list of preregistered fundamental datatypes.
        		},
        	},
    	}
    );
    // Or this
    sr.registerTemplated<StructHere>("CustomNameOfStruct");
    // We can ensure that we only get the structs we want from
    // our struct registry by extracting it.
    graph.findSourcesRecursiveStructs(sr.filterRegex("Int*"));

    // Future? Find templates of structs, making use of some c++ api
    // thing for deduction?
    // graph.findSourcesRecursiveStructsTemplated(std::vector<TemplatedStructType>...)

    // Now do whatever you want for visualising the graph
    for (const auto& link : graph.linkIterator())
    {
        ...
    }
    for (const auto& node : graph.nodeIterator())
    {
        ...
    }
    // Or you can detect cycles.
    // But i'm not exactly sure how to represent this.
    auto cycles = graph.findCycles();
    // Or extra stuff.
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
BATCHER = rmf.Batcher(6) # pre injected / run within the embedded interpreter before everything.
REGIONS = rmf.parseMaps(PID) # pre injected / run within the embedded interpreter before everything.

# Filters for regions actively in memory, and then breaks them into chunks with sizes 0x1000
chunkedRegions = REGIONS.filterActiveRegions().breakIntoChunks(0x10000, overlap=0)
snapshots = BATCHER(rmf.makeSnapshot, chunkedRegions)

results = BATCHER.findString(snapshots, "hello world!").flatten()

results.batchModify(sizeDiff=+0x100, addrDiff=-0x50)

# Dump the results into a graph
mg = rmf.MemoryGraph()
mg.pushNodes(results)

# Create links between nodes that exist in the graph.
# Does this naively by analyzing 8 byte chunks within each
# region and sees if it links into or inside another region
# already in the graph.
mg.findLinksStrict(BATCHER)

rmfg.pushMemoryGraph(mg, policy="combine")
```

Finding changing regions (as an extension on what happened previously
```python
import rmf_py as rmf # preinjected / run within the embedded interpreter before everything.
import rmf_gui_py as rmfg # preinjected / run within the embedded interpreter before everything.
PID = # pre injected / run within the embedded interpreter before everything.
BATCHER = rmf.Batcher(6) # pre injected / run within the embedded interpreter before everything.
REGIONS = rmf.parseMaps(PID) # pre injected / run within the embedded interpreter before everything.

chunkedRegions = REGIONS.filterActiveRegions().filterHasPerms("rw").breakIntoChunks(0x10000, overlap=0)

from copy import deepcopy
results = deepcopy(chunkedRegions)

from time import sleep

while len(results) > 10:
    snaps1 = BATCHER.makeSnapshot(results)
    sleep(1) # wait 1s
    snaps2 = BATCHER.makeSnapshot(results)

    results = BATCHER.findChangedRegions(snaps1, snaps2, 32) # using 32 byte chunks.

# Only modify relative parameters obviously
results.batchModify(sizeDiff = +10, addrDiff = -10)
# or we can set them to be batchStructified?

mg = rmfg.pullMemoryGraph()
mg.pushNodes(results)

# Relaxed, so creates nodes which point to existing nodes.
# internally will update, but also will return a list of new mrps generated
mg.findLinksRelaxed(BATCHER, REGIONS)

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

# Graph implementation

The datastore is separate, under MemoryGraphData.
You can add, delete and update to this, and other stuff.
In fact you can combine multiple together, but no method operations
can be applied to this.

MemoryGraphData will have operations under rmf::op::*(const MemoryGraphData&, ...)
These operations should be pure, at least to some degree.
These return more MemoryGraphDatas that can be added together?
The only problem is that we are constantly going to have to copy and combing
graphs together, which is slow. There are some vector abstractions like
std::ranges that could help with this, but I am not sure on how to ensure
smooth operations. Also what would we do with links? We can't really add links together,
as the ids will differ unless we use uuids.

MemoryGraph is the actual API that users will use. It will contain `sptr<MemoryGraphData>`
Has methods like the ones shown above on populating itself.
