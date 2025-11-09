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

# Methodology

- Read the memory map of the process, and then perform a search. For a
  manually changeable target (like a player's movement):

1. Constantly change the target's value, and search through each writable
   region, taking 2 snapshots at a designated interval and look for changes.

2. Store said changes, and then use them to generate sub regions.

3. Poll the sub regions again for **NO** changes specifically, and remove
   regions that had changed

4. Rinse and repeat until we have found the desired region.

Using the found desired sub region, an api is available which would let us poll
said variable. Checks will be done every poll in order to ensure that the
memorymap has not changed.

# TODO:
- Complete the GUI, alongside saving and loading sets of instructions representing
    actions taken.

- Write a parser for the instructions. (Super simple, no need for syntax checking etc.);

- Simplify certain aspects of the API.
    Make it so I don't have to write 60 lines of code to setup a multi-threaded change detection.
    On a whole region.

- Add extra core functions and pointer finding.
    maybe even linked list detection with a range of pointers given.

# issues
- There seems to be some problem with the logging system in multi-threaded contexts,
    which completely prevents any logs from showing if there are a large amount of tasks.
    I'm pretty sure snapshotting works correctly, however.

# language guide:
## Basic Features
Index : Supports negative indices, with -1 being the most recent.
Basic mathematics support (float serialisation, basic arithmetic)
Variable access in the rpl timeline (actualRegionStart, size)
Valid Instructions:
```
ReadMaps (no arguments)

CopyOriginalMaps (no arguments)

FilterRegionName "Name" index

FilterRegionSubName "SubName" index

FilterPerms "Perms" index

FilterHasPerms "Perms" index

FilterNotPerms "Perms" index

FilterActiveRegion index

MakeSnapshot register index

FindChangingRegions Register1 Register2

FindUnchangingRegions Register1 Register2

FindString Register1 "string"

// Use FindNumeric for pointers as well.
// How can I allow for more customized variables?
// The Min Max values are guaranteed to not be known at compile time,
// So we must be able to access values of maps.
FindNumeric Register1 Min Max

FindChangingNumeric Register1 Register2 MinDiff

FindUnchangingNumeric Register1 Register2 MaxDiff

Loop instructionListIndex until stable NumUnchanged
Loop instructionListIndex until X remain

// Tag a specific region as high-interest, and will be stored
// in a special high-interest buffer with refreshing capabilities.
Tag timelineIndex regionIndex

// Maybe add helpers for numerics for pointers, linked list and maybe structs.

// An instruction is started and ended using START and END keywords before each clause.
```
Example file
```
START

ReadMaps // Read the maps
FilterPerms "rwp" -1 // Only accept regions with these properties ONLY.
FilterActiveRegion -1 // Remove all regions of memory not actively in memory

END


START

// Can only be started by some user input.
// In headless mode it waits for the bool "Paused" to be
// set to false.
PAUSE 

MakeSnapshot r1 -1
Sleep 500ms
MakeSnapshot r2 -1
FindChangingRegion r1 r2

PAUSE 

MakeSnapshot r1 -1
Sleep 500ms
MakeSnapshot r2 -1
FindUnchangingRegion r1 r2

END

START

// Relative index, so -1 is the one before.
Loop -1 until stable 4
Tag -1 0

END
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
