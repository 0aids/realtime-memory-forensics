# Memory analysis

A tool for recognising patterns within a memory for fast identification of
perhaps hidden information that is required.

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
