# Example case
A small program, containing a pseudo garbage collector, and an array of float3s stored contiguously.
This array is pointer to by a "player" struct, containing a name and said pointer.
Everything here is also pointed to by the garbage collector. (I have no clue how exactly a garbage collector
works, this is just how it works in my mind).
```c
typedef struct {
    float x, y, z;
} float3;

typedef struct {
    float3 head[]; // allocated dynamically
    size_t count;
} float3_arr;

typedef struct {
    const char* name; // Allocated dynamically
    float3_arr bodyPositions;
} Player;

// pseudo garbage collector
typedef struct {
    void* targets[];
    size_t count;
} garbage_storage;
```
For this simple example, we already know about all 3 of these cases.
