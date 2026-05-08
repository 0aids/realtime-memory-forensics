# Realtime Memory Forensics (rmf)
Realtime memory debugger/analysis tool for hooking and analyzing the structure
of actively running linux processes, without interrupting via ptrace.
Designed high-throughput without sacrificing clean API usage.
Planned python-based DSL JIT, and visualisation tools.

# Currently available features
1. Full process memory captures.
2. Map filtering
3. Multi-threaded memory searches and operations
4. Direct raw memory access.

# Planned Features
- [-] Type registry for using structs to help with analysis.
- [ ] Memory graphs - The megastructure holding information
        on process's pointer graph.
- [ ] Python bindings
- [ ] Raylib based visualisers for more visual analysis.
- [ ] Schema based searching?

# TODO
- [-] Vector method piping chaining operations
- [ ] Basic type parsing
- [ ] Redone struct registry
- [ ] Typed mixin for region
- [ ] Attempt 3 for memory graphs.
- [ ] Visualiser for memory graphs using raylib.
- [ ] Allow different nodes for binary operations.
- [ ] Write python bindings
- [ ] done for now?
- [ ] Consider serialisations
- [ ] Test out multi-SPSC queues instead of SPMC queues for tasks? Or provide it as an alternative.
      Literally was just a random thought but it seems like it would be faster. Apparently worse load balancing,
      which might not be a problem in this case? the Go scheduler uses work stealing as well, which
      might also be another interesting thing to consider. Cyclic arrays for queus are DEQUEs anyways.

# Building
Dependencies automatically fetched via cmake.
```bash
cmake -S . -B build -Dtests=... -DUBsan=... -DAsan=... -DcompileExamples=... -DCMAKE_BUILD_TYPE=...
cmake --build build -j $(nproc)
```

# Running tests
```bash
cmake -S . -B build -Dtests=ON && cmake --build build -j 12 && (ulimit -m 1000000 && ulimit -v 1000000 && cd build && ctest)
```

# Planned C++ api
```cpp
/****************************/
/****** Create Structs ******/
/****************************/
mf::TypeRegistry sr;

// Builder pattern with explicit field deduction?
mf::Struct node = sr.defStruct("Node"/*, sr.PACKED or sr.DEFAULT*/)
    .field(sr.prim.u32, 		 "data")
    .field(sr.ptrTo(sr.struct_("Node")), "next")
    .field(sr.arrOf(sr.u32, 10), "array")
    .field(sr.arrOf(sr.ptrTo(sr.struct_("Node")), 10), "pointers")
    .field(sr.struct_("PredefinedStruct"), "name")
.end(); // Full type is resolved here, offsets calculated, etc.
// maybe you could call "defStruct" inside of the builder if I feel like implementing it.
// Maybe in the future also allow defining types before hand and then creating
// structs from them.

// Or nested structs and chars
mf::Struct bytes100 = sr.defStruct("Bytes100")
    .array("uint8_t[100]", "data")
.end();

mf::Struct vec3d = sr.struct_("vec3d")
    .field("float", "x")
    .field("float", "y")
    .field("float", "z")
.end();

mf::Struct customVec2dArr = sr.struct_("Vec2dArr")
    .struct_("Vec2d")
        .field("float", "x")
        .field("float", "y")
    .end()
    .field("Vec2d*", "data")
    .field("size_t", "length")
.end();

mf::PartialStruct partial = sr.struct_("Programatically");
// Programatically create structs
for (size_t i = 0; i < 10; i++)
{
    partial.field("uint8_t[{}]"f.fmt(i), "field{}"f.fmt(i));
}
mf::Struct programatically = partial.end();

// Or mf::Struct with explicit cast via ".as<mf::Struct>()"
mf::Type vec2dStruct = customVec2dArr["Vec2dArr"];

// Get nested struct value if possible
mf::Type vec2dStruct = sr["Vec2dArr", "Vec2d"];
size_t vec2dAlignment = vec2dStruct.alignment;
size_t vec2dAlignment = vec2dStruct.size;

// You can also get a field and it's data.
// A field contains a type and a parent.
mf::Field vec3dY = sr["vec3d", "y"];

// We can reshape Maps based off fields and other stuff. See below.
/***************************************/
/******* Basic Memory Operations *******/
/***************************************/
ThreadPool tp(std::this_thread::hardware_concurrency()); // Multithreading
pid_t pid = ...;
using namespace mf;

// Get our original maps.
Vec<Node<Map>> maps = getMaps(pid)
    .minSize(0x1000)
    .maxSize(0xffffff)
    .active(pid);

// Most pure operation functions provide a threaded method, which works multithreaded.
Vec<Node<Map, Snapshot>> snapshots = makeSnapshot.threaded(maps, pid).with(tp);

// Alternatively
Vec<Node<Map, Snapshot>> snapshots = maps.mapThreaded<Snapshot::captureM>(pid).with(tp);

// If we want to work on an individual node, we can do the following
Node<Map, Snapshot> snapshot = maps[0].capture(pid);

// Obviously we can just access the data raw
mf::Node<Map, Snapshot> snap1 = snapshots.front();

// find* are static functors that support operator(), or a .threaded version which takes in an analyzer.
Vec<mf::Node<Map>> stringInSnap = findStr.threaded(snapshots, "RandomString!").with(tp);
Vec<mf::Node<Map>> floatYRanges = findNumInRange<float>.threaded(snapshots, 0.99, 1.01).with(tp);
Vec<mf::Node<Map>> numCloseTo = findNumCloseTo<double>.threaded(snapshots, 1e5, 0.5).with(tp);
Vec<mf::Node<Map>> strLike = findStrLike.threaded(snapshots).with(tp);
Vec<mf::Node<Map>> exactNum = findNum<uint32_t>.threaded(snapshots, 1000).with(tp);

// We can also mass resize or get the names of all the maps.
// We use this implementations which are the capital letter'd versions.
Vec<mf::sptr<const std::string>> names = vecMaps.map<Map::getNameM>();

Vec<mf::Node<Map>> resized = vecMaps.map<Map::resizeM>(MapDelta{.offset=-0xff, .deltaSize=0xff});

// You can coerce results and then extract values.
// Say for example our floats we found are expected to be Y values in a vec3d.
// mfu::vec has specialisations for certain types that automatically parallelise.
Vec<mf::Node<Map, Typed>> vec3dMap = floatYRanges.map<Field::fromFieldM>(vec3dY);

// We can also access data of nodes.
mf::Node<mf::Map, mf::Typed> node = vec3dNodes.front();

// A node that owns snapshot data from pid.
mf::Node<Map, Snapshot, Typed> nodeWithCapture = node.capture(pid);
mf::Node<Map, Snapshot, Typed> nodeProperty = nodeWithCapture.property("x");

// Or equivalently with less capturing
mf::Node<Map, Typed> nodeProperty = node.property("x");
mf::Node<Map, Snapshot, Typed> wideProperty = nodeProperty.capture(pid);

// Entire schema to view a field
float xValue = StructType
                .getField("x")
                .nodify(node)
                .capture(pid)
                .typedAs<Primitive>()
                .as<float>();

// Similar.
float yValue = StructType
                .nodify(node.capture(pid))
                .nodeAtField(vec3dy)
                .typedAs<Primitive>()
                .as<float>();

// We can pipe vectorised method operations together.
// What if any one of these fails? For example attempting to
// type a struct as a primitive? Should it throw should typedAs return
// an optional? I think for now we will just throw an exception
// (exceptions are never caught)
// Ideally in the future all chained options should take in optional values,
// and then forward all errors.
Vec<float> xValues = vec3dNodes.pipe() |
                         Struct::nodeAtFieldF("x") |
                         Snapshot::captureF(pid) |
                         Typed::typedAsF<Primitive>() |
                         Primitive::asF<float>() |
                     Pipe::end;

Vec<float> xValues = vec3dNodes.pipe() |
                         Struct::nodeAtFieldF("x") |
                         Snapshot::captureF(pid) |
                         Typed::typedAsF<Primitive>() |
                         Primitive::asF<float>() |
                     Pipe::endThreaded(tp);
                     // Similar, but also allows threading.

// To be updated as more features are added.
```

# Planned Python API
```python
from rmfpy import getMaps, ThreadPool, Op, Primitive
from rmfpy.types import TypeRegistry
import rmfpy as mf

tr = TypeRegistry()
Node = tr.struct("Node").field("uint32_t","data").field("Node*","next").end()
Vec3 = tr.struct("Vec3").field("float","x").field("float","y").field("float","z").end()
Vec3_y = Vec3["y"]

tp = ThreadPool(8)
PID = 1234
maps = getMaps(PID).minSize(0x1000).maxSize(0xffffff).active(PID)
snaps = maps.capture(PID, threaded=tp)

strings  = snaps.findStr("RandomString!", threaded=tp)
floats   = snaps.findNumInRange(mf.f32, 0.99, 1.01, threaded=tp)
close_d  = snaps.findNumCloseTo(mf.f64, 1e5, 0.5)
exact    = snaps.findNum(mf.uint32, 1000, threaded=tp)

vec3d_maps = floats.fromField(Vec3_y)         # coerce to Vec3

x_vals = (vec3d_maps
          .pipe()
          .nodeAtField("x")
          .capture(PID, threaded=tp)
          .typedAs(Primitive)
          .asFloat())
```
