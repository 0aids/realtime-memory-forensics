# Realtime memory forensics
Realtime memory analysis for currently running applications.

# Todo
- [ ] Implement memory region mixins
    - [ ] Design and figure out how they should be used
    - [ ] Implement mixins
    - [ ] Write tests for mixins
- [ ] Implement python API
    - [ ] Implement direct translations of core data structures and methods
    - [ ] Implement sugar over those
    - [ ] Tests
- [ ] If python API is slow, create more python API over some multi-SPSC work-stealing threadpool


# Python API
Python serves as a controller for manipulating c++ objects to have a good balance between API usability
and speed.
```python
import rmfpy as mf
# functors for vectorised operations
from time import sleep

proc = mf.Process(pid = ...)

# Node<Map>
# Make types automatically have relevant vectorised functors added as methods
maps = proc.readMaps().getActive(proc)

class PlayerStruct(mf.Struct):
	name: mf.Ptr[mf.u8]
	x: mf.f32
	y: mf.f32
	z: mf.f32

# Node<Map, Snapshot>
memoryRegions = proc.getRegions(maps)


candidates = (memoryRegions
    .findString("xXcoolGuyXx") # Node<Map, Snapshot> -> Node<Map, Snapshot>
    .findPtrsToSelfFrom(memoryRegions) # Node<Map, Snapshot> -> Node<Map, Snapshot>
    .structifyFromField(PlayerStruct.name) # Node<Map, Snapshot> -> Node<Map, Snapshot, Struct>
)

# get candidates whom's x values are between 101.5 and 102.5
playerCandidates = candidates.where(candidates.PlayerStruct.x.between(101.5, 102.5))

# And so on for analysing stuff.
# Eventually you should be able to relatively deterministically determine the location of key interests of memory
# by performing these operations repeatedly.
# Node<Map, Snapshot, Struct, Proc>
playerFinal = ...

# You can then consistently easily access key parts of this memory
for _ in range(1000):
    playerFinal.refresh()
    print(playerFinal.getStruct())
    # prints:
    # PlayerStruct
    #     - Name : "xXcoolGuyXx"
    #     - x : 102.3
    #     - y : 98.31
    #     - z : 0.32
    sleep(0.1)

# You can also use it to perform discoveries on other data structures.
memoryRegions = proc.getRegions(maps)

class GameKernelStructGuess(mf.Struct):
    unknown: mf.Array[mf.Ptr[mf.v0], 10]

# Search for a pointer to our struct
gameKernelCandidates = (playerFinal
    .findPtrsToSelfWithinRangeFrom(-0xff, +0xff, memoryRegions)
    .findPtrsToSelfFrom(memoryRegions)
    .# Some more checks and what not
    .structifyFromField(GameKernelStructGuess.unknown[5])
    )

# And so on.
# Maybe for guesses we should be able to modify game structs accordingly, but this is probably not feasible.
# But that stuff is beyond me for now.
```

# AI policy
Only for helping writing tests. (Because writing tests is boring)
