
```cpp
/***************************************/
/******* Memory graph operations *******/
/***************************************/
// Templated, using anything that satisfies the concept of a node, and link.
// This means that we can then use a special modified node that has extra metadata for guis.
mf::MemoryGraph mg; // holds sptr.

// After we're happy with this, we can move the data to mg.
// Has diffs?
// Duplicates are ignored, but might print a warning.
// ??: What happens to parts that are connected but separated by mgps? We will still have them connected, as it's connected in the single source of truth.
// mgp holds a wptr to mg_impl, and vectors of NodeKeys and LinkKeys to their relevant slotmaps (references cannot be used because the datastructure is
// constantly growing and shrinking and changing, so they will be invalidated incredibly easily).
// However, mgp will have wrapper functions that would make it easier and friendlier on operations.
// mgps use the widest available nodes.
// Using Node<Map, Snapshot, Typed> (and maybe more for storing relations?)
mf::MemoryGraphPart mgp = mg.push(vec3dNodes);

mf::MemoryGraphPart mgp = mg.findSources(snapshots, nodeKeys, sourceField);
mf::MemoryGraphPart mgp = mg.findSources(snapshots, nodeKeys, sourceField, targetField);

// The node being used will have to have pointer values.
mf::MemoryGraphPart mgp = mg.findTargets(snapshots, sourceNodeKeys, targetField);

// Removes Link-less nodes
mf::MemoryGraphPart mgp = mg.pruneLinkless();

// Removes links and nodes that have changed.
mf::MemoryGraphPart mgp = mg.pruneExpired(newSnapshots);
mf::MemoryGraphPart mgp = mg.pruneExpired.threaded(newSnapshots).with(tp);

// Multithreaded operation. findSources is not actually a function, but a class.
// The class has operator() and operator[]. operator() acts like a normal function
// operator[] acts like a weird function, returning a special type with another operator() awaiting an analyzer.
mf::MemoryGraphPart mgp = mg.findSources[snapshots, nodeKeys, sourceField](tp);

// Equivalent to:
mf::MemoryGraphPart mgp = mg.findSources.threaded(snapshots, nodeKeys, sourceField).with(tp);

// What about traversal? of course can traverse.
mf::MemoryGraphPart children = mg.getChildren(moreKeys[0]);
mf::MemoryGraphPart parents = mg.getParents(moreKeys[0]);
// No nodes, contains link references.
mf::MemoryGraphPart outgoingLinks = mg.getOugoingLinks(moreKeys[0]);

// Filtering is easy as well.
mf::MemoryGraphPart mgp = mg.filterType(vec3d);
mf::MemoryGraphPart mgp = mg.filter*(...);

// Grabbing a node and checking their values.
Vec<float> floats = mgp.nodes[0].capture.threaded(pid).property("x").as<float>().with(tp);

// Or we can just get nodes with embedded snapshots.
// But we do lose link information??
Vec<mf::FullNode> vecFullNodes = mgp.nodes[0].capture.threaded(pid).with(tp);

// using "e" to extract elementwise properties.
Vec<float> floats = vecFullNodes.e.property("x");

// possible undos?
mg.pop();
```

# Memory graphs
planned design
```cpp
// mf::field has an sptr underneath.
// mgr stores nodes in a slotmap, not a vector, so we can
// have invalidatable references if needed.
// Adds stuff
template <NodeContains<Map, Typed> node_t>
class FieldLink
{
	mf::Field sourceField;
	mfu::SMapRef<node_t> source;

	mf::Field targetField;
	mfu::SMapRef<node_t> target;
}

struct Typed {
	mf::Type type;
	// Relevant functions with deducing this.
	// ...
}

// Another mixin for linked
// Burning questions:
// - How are we initialised?
//     Consider an "after" block that happens for copy and move.
// - How do we referecnce self or the head of a struct?
//     No clue. Have custom fields representing the heads?
// - What about partially identified structs?
//     No clue. Have custom fields representing the unknown?
template <NodeContains<Map, Typed> node_t>
struct TypeLinked {
	std::unordered_map<mf::Field, mfu::SMapRe<mf::FieldLink<node_t>>> outgoingLinks;
	std::unordered_map<mf::Field, mfu::SMapRe<mf::FieldLink<node_t>>> incomingLinks;
	mfu::SMapRe<node_t> selfRef;
	// Relevant functions
	// ...
}

// Custom links
links.in(field) // returns the incoming link for that field
links.out(field) // returns the outgoing link for that field

// consider swapping from full node to something templated.
class MemoryGraph
{
	struct MGData {
    	mfu::SlotMap<mf::FullNode> nodeStorage;
    	mfu::SlotMap<mf::FieldLink> linkStorage;
	};
	sptr<MGData> m_data;
	// Relevant pushing and inspection and iteration functions.
	// Has all the relevant filtering options, but returns
	// map references instead.
}

class MemoryGraphPart
{
    // Memorygraphs use shared pointers to hold data.
	MemoryGraph mg;
	Vec<mfu::SMapRe<mf::FieldLink>> links;
	Vec<mfu::SMapRe<mf::FullNode>> nodes;
	// Similar but not the same? Only supports inspection, modification
}

// We only ever render using parts. The mg serves as the main storage and source of truth.
void renderMemoryGraphPart(const MemoryGraphPart& mgp) {
	// Raylib stuff.
}
```
