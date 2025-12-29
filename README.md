# Realtime Memory Forensics (rmf)
Fun!

# Operations
## Snapshots
### Binary
Find Changing Regions
Find Changing Numeric Region: Minimum and maximum amounts

### Unary
Find strings
Find Numeric Exact
Find Numeric within Range
Find pointer within region/s
Find structs using pointers

## Region Properties
Chunking into smaller bits.
Modifying region sizes

## Gui
Updatable snapshots with different data views.

## Regions
A region contains:
1. Metadata
2. Customizable label
3. Snapshot data buffer
4. List of incoming pointers
5. List of outgoing pointers

This will be used to draw node graphs to see the data flow
of the program.
