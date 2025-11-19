# Memory analysis

A tool for recognising patterns within a memory for fast identification of
perhaps hidden information that is required. Super fast, multi-threaded support,
(and maybe GPU support later).

# GUI
An interface that allows exploration of snapshots of a program's memory.
Allows identification of high-interest areas in memory, as well as may help
identify high-interest structs or values.
Runs on a bunch of "Commands" which can be saved and run without the GUI
later to reproduce locating of high-value targets.

# API
An interface to allow exploration of memory. By itself is too complex to use, but
when coupled with the Instructions and abstractions, provides a reliable way to
retrieve valuable targets in memory.

# Notes before running

- Ensure that yama is set to 0 via

  ```bash
  echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
  ```

  And reset it afterwards by echoing `1` instead.

# TODO:
- Complete the GUI, alongside saving and loading sets of instructions representing
    actions taken.

- Write a parser for the instructions.

- Add extra core functions and pointer finding.
    maybe even linked list detection with a range of pointers given.

# issues
- There seems to be some problem with the logging system in multi-threaded contexts,
    which completely prevents any logs from showing if there are a large amount of tasks.
    I'm pretty sure snapshotting works correctly, however.

# Scripting language guide:
```python
# Everything is typed, automatically.

# A pid of -1 is a debug pid - When reading the maps it will
# automatically change to the value of the sample process.
pid = -1;
chunkSize = 0x100000;
numThreads = GetConcurrency() - 1;

backend = MTBackend(numThreads);

allMaps = ReadMaps(pid)

print("Sample of maps:\n{}", allMaps[0]);

filteredMaps = filterByPerms(allMaps, "rwp");

delete allMaps;

# initiate types via colon operator.
results : RegionPropertiesList;

for region in 0:chunkSize:filteredMaps.size()  # Not Inclusive of 1000, intervals of 50, etc.
    # The section:
    # "filteredMaps[region::chunkSize]" will create a temporary
    # view of range region::chunkSize, which is a region that starts
    # from index "region" and up to "region + chunkSize".
    # If the indices exceed the filteredMaps' size, then the
    # bounds will automatically be conformed.
    snap1 = backend.makeSnapshot(filteredMaps[region::chunkSize]);
    sleep 50ms;
    snap2 = backend.makeSnapshot(filteredMaps[region::chunkSize]);
    results.append(backend.findChangedRegions(snap1, snap2));
end
# After a script finishes, all values in scope can be accessed and viewed.
# Via the GUI Menu, or retrieved via an API.
# The API Will allow access to scoped variables returning a unordered map of pairs of
# {"symbol name":string_view, const &type value}.
# From which can be copied into actual owning values if needed.

```

# Styling guide for myself to be consistent

```cpp
// Enums are to be done with the prefix e_Name
enum e_ThisEnumeration{};
// All classes including enums use PascalCase
// This includes namespaces
namespace MyNamespace {
class ThisClass{};
};

// All other variables are camelCase
class ThisClass::thisFunc();
int functionDefinitionA();
int stupidValue = 0;
for (int incrementVariable = 0; i < 100; i++) {
    stupidValue += incrementVariable;
}

// A class's member variables are prefixed with m_
// There is an exception for C styled structs (no member functions)
class ThisClass {
private:
    int m_numberOne = 1;
public:
    int m_whoDoesPublicVars = 0;
};

struct ThisStruct {
int var0;
int myVar;
}


// Constants use SCREAMING_SNAKE_CASE
const int MAX_VALUE = 100;

// Filenames are snake_case
#include "some_random_header.hpp"

// If it is a list or a vector of some sort, suffix with _l
std::vector<ints> intsList_l

// For preprocessor definitions, add the prefix p_ to it, and use snake case
#define p_var_debug(...) ...
// Unless they are used for constants, in that case use SCREAMING_SNAKE_CASE
```

# AI disclosure

AI has been used to:

1. Help with questions that cannot be conventionally asked to google.
2. Help me with making some design decisions (IE: Asking for a specific DS that might suit my goals)
3. Help me debug when I'm at my wit's end

Any exceptions to the rules above are disclosed in their specific file.
