# Program structure

A MemoryRegion has a list of snapshots of the entire rgion, as well as a list of subregions.
These subregions are used to track specific subregions of memory. We need

# TODOS

Create a memory region abstract base class, and then the proper memory region with sub regions. Add a wrapper to a list of sub regions called subregion list, used for keeping tracks of subregions within a bigger region.
