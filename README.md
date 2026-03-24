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
- [+] fix: Find that stupid race condition
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
can be applied to this. It is normally owned by MemoryGraph for user access,
which has helper methods to help modify its internal data.

The key detail is OrderedSnapshot, which just a sorted vector of { target, sourceMrp }
which we can then perform binary searches when we want to find regions that point to
specific other regions.

Using this, we can do findSourceRegions which will return a vectors of pairs
{ sourceMrpVec, targetMrpVec }.

It is also advisable to add a struct registry into a memory graph so that we can resolve
links. Links will not exist unless structs are given pointers. The default one is (void*)
if we don't know what type we are pointing to.

Now we can just add this to our MemoryGraph by doing mg.addLinks(source, target, policy::replace)
They are implicitly converted into nodes, but it would be nicer if they were converted into
nodes earlier so that we can assign a default pointer struct to it. Under the hood
it would manage all the nodes and generate specialised link structs so we know what
links to what.

MemoryGraphData should be holding a sorted vector of Nodes, sorted by TrueAddress.
It also holds a vector of links. The link holds data about who is the source, which
element of the struct is the source, and the target, and if the target is another where/
what it points to in the other struct.

When we do mg.addLinks(source, target), the MemoryGraph will do some internal calculations
using MemoryGraphData to try to insert sources (and if they exist or replace them depending on the policy),
and also try to insert

We should also have an internal struct registry used by the memory graph.
The links will store all the struct data about which part of the struct points to what.
It would sort of be in the form of like
```cpp
struct LinkData {
	StructMemberId source_smi;
	StructMemberId target_smi;
	uintptr_t sourceAddr;
	uintptr_t targetAddr;
	...
}

struct NodeData {
	MemoryRegionProperties mrp;
	StructTypeId structType;
	...
}
```

What is the goal of MemoryGraphs?
1. To help provide easier visualisation of data flow
    a. By doing so, help find methods of deterministically finding the key structure of a program
    b. To profile a program's use of temporary objects
2. To help provide more deterministic ways of finding the key structure of a program
    a. By doing so, allowing us to reverse engineer and consistently find key areas of a programs memory
    b. Offering solutions for exploring and deciphering pointer chaining

For each of the goals, what things do I need to consider?
1. How efficient should removals and additions be?
	Efficient as we want to be able to update the graph when volatile parts disappear or
	no longer reference correctly. O(1) for insertion and deletion

2. How efficient should iterating / access be?
	Efficient as we want to be able to draw this graph containing 10,000s of nodes.
	O(n) for full iteration? But node link deduction should be O(log n)

3. How efficient should searching be?
	Somewhat important, as we could want to filter nodes with specific properties
	But I think O(n) is fine in this case.

4. How efficient should traversal be?
	Not sure. I don't know exactly what's the best workflow.
	O(1) to find the next node if using cache, but generating the
	cache might take O(n^2 log n), and the cache is immediately invalidated
	if the graph updates.

How should node and link resolution work?
There are a couple of options on how to do this, but the main thing
is that the MemoryGraph has no clue on whether or not the data is linked.
So we need to tell it that 2 nodes are linked, and tell it to link them.

Who is in charge of doing the link and node resolution?
We are in charge of finding the links, and then telling the memory graph that
specific nodes are linked together.

For example, we could do the following:
1. Select a set amount of nodes from our memory graph
2. Use those nodes to find other nodes that link to our nodes.
3. Tell our memory graph that other nodes link to these nodes.

So where should links be traversed from? Each node can have multiple links,
and links need to have some data, so a central storage is required.
1. Links are stored in a sorted array, sorted by source address and target address.
2. Links are stored in a std::unordered map, with addr as keys
	- The thing with this is that traversal will be faster if keys are
	  pointing directly to known areas
2. Links are stored in a std map
	- Slowest? 

As for structure members, we can deal with that later.

What about updating the structure?
If we update a node, it may or may not invalidate the links pointing to it.
IE if we update the node to be larger, then we must modify the links pointing to it.
Similarly with links?

We'll just invalidate the links.
