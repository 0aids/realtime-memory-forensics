#include "snapshots.hpp"
#include "logs.hpp"
#include "threadpool.hpp"
#include <algorithm>
#include <iterator>
#include <unistd.h>
#include <cstdint>
#include "executor.hpp"
extern "C"
{
#include <bits/types/struct_iovec.h>
#include <sys/uio.h>
}

const MemorySnapshot MemorySnapshot::Builder::build()
{
    return MemorySnapshot(std::move(data),
                          std::move(regionProperties),
                          std::move(timeCaptured));
}

void makeSnapshotCore(MemorySnapshot::Builder& build,
                      MemoryPartition          partition)
{
    Log(Debug, "Taking snapshot");
    using namespace std::chrono;

    const uintptr_t startAddr =
        build.regionProperties.getActualRegionStart() +
        partition.relativeRegionStart;
    const std::string& name = build.regionProperties.regionName;
    const uintptr_t    size = partition.size;
    struct iovec       localIovec[1];
    struct iovec       sourceIovec[1];

    Log(Debug, "Region name: " << name);
    Log(Debug,
        "Num bytes for this task: " << std::showbase << std::hex
                                    << size);
    Log(Debug,
        "Actual Start address: " << std::showbase << std::hex
                                 << startAddr);
    Log(Debug, "Size: " << std::showbase << std::hex << size);

    const size_t chunks = 1 << 24;

    int64_t      totalBytesRead = 0;

    while (totalBytesRead < static_cast<int64_t>(size))
    {
        uint64_t bytesToRead =
            (size - totalBytesRead > (ssize_t)chunks) ?
            chunks :
            size - totalBytesRead;
        sourceIovec[0].iov_base = (void*)(startAddr + totalBytesRead);
        sourceIovec[0].iov_len  = bytesToRead;

        localIovec[0].iov_base = build.data.data() + totalBytesRead +
            partition.relativeRegionStart;
        localIovec[0].iov_len = bytesToRead;

        // This is a lot simpler than using lseek, and permissions are granted (no
        // sudo) if yama is 0
        ssize_t nread =
            process_vm_readv(build.regionProperties.pid, localIovec,
                             1, sourceIovec, 1, 0);
        if (nread <= 0)
        {
            if (nread == -1)
            {
                Log(Error,
                    "Completely failed to read the region. Error "
                    "is below.");
                perror("process_vm_readv");
                return;
            }
            Log(Error,
                "Read " << nread << "/" << size
                        << "bytes. Failed to read all the bytes "
                           "from that region.");

            return;
        }
        totalBytesRead += nread;
    }
}

// Modifies only its build. The build needs to be consolidated later.
void findChangedRegionsCore(RegionPropertiesList::Builder& build,
                            uintptr_t compareSize,
                            MemoryPartition mp,
                            const MemorySnapshot& snap1,
                            const MemorySnapshot& snap2)
{
    uintptr_t current = mp.relativeRegionStart;
    Log(Debug,
        std::hex << std::showbase
                 << "Finding changing regions, relativeStart: "
                 << current << ", " << current + mp.size);
    Log(Debug,
        "Number of iterations required: " << mp.size / compareSize);
    for (size_t i = 0; i < mp.size / compareSize; ++i)
    {
        uintptr_t curSize = (i == (mp.size / compareSize) - 1) ?
            (mp.relativeRegionStart + mp.size) - current :
            compareSize;

        if (memcmp(snap1.data() + current, snap2.data() + current,
                   curSize))
        {
            Log(Message,
                "Found a change @ address"
                    << std::hex << std::showbase
                    << snap1.regionProperties.parentRegionStart +
                        current);
            if (build.data.size() > 0 &&
                build.data.back().relativeRegionStart +
                        build.data.back().relativeRegionSize ==
                    current)
            {
                build.data.back().relativeRegionSize += curSize;
            }
            else
            {
                MemoryRegionProperties mrp = snap1.regionProperties;
                mrp.relativeRegionSize     = curSize;
                mrp.relativeRegionStart    = current;
                build.data.push_back(std::move(mrp));
            }
        }
        current += curSize;
    }
    Log(Debug, "Found changes: " << build.data.size());
}

MemorySnapshot makeSnapshotST(MemoryRegionProperties properties)
{
    MemorySnapshot::Builder build(properties);
    return makeGenericST<MemorySnapshot>(
        build, makeMemoryPartitions(properties, 1).front(),
        makeSnapshotCore);
    // makeSnapshotCore(build, {0, properties.relativeRegionSize});
    // return build.getResult();
}

RegionPropertiesList findChangedRegionsST(const MemorySnapshot& snap1,
                                          const MemorySnapshot& snap2,
                                          uintptr_t compareSize)
{
    RegionPropertiesList::Builder build;
    return {};
    // return makeGenericST<RegionPropertiesList>(
    //     build,
    //     makeMemoryPartitions(snap1.regionProperties, 1).front(),
    //     findChangedRegionsCore, compareSize, snap1, snap2);
}

MemorySnapshot makeSnapshotMT(MemoryRegionProperties properties,
                              ThreadPool&            tp)
{
    MemorySnapshot::Builder build(properties);
    return makeGenericMT<MemorySnapshot>(
        tp, build, makeMemoryPartitions(properties, tp.numThreads()),
        makeSnapshotCore);
}

RegionPropertiesList findChangedRegionsMT(const MemorySnapshot& snap1,
                                          const MemorySnapshot& snap2,
                                          ThreadPool&           tp,
                                          uintptr_t compareSize)
{
    // This one is a structure supports the BuildConsolidator concept.
    auto parts =
        makeMemoryPartitions(snap1.regionProperties, tp.numThreads());
    RegionPropertiesList::Consolidator consolidator(tp.numThreads());

    return {};
    // return makeGenericConsolidateMT<RegionPropertiesList>(
    //     tp, consolidator, parts, findChangedRegionsCore, compareSize,
    //     snap1, snap2);
}

std::vector<MemorySnapshot>
makeLotsOfSnapshotsST(RegionPropertiesList rl)
{
    std::vector<MemorySnapshot> results{};
    results.reserve(rl.size());

    for (const auto& properties : rl)
    {
        MemorySnapshot::Builder builder(properties);
        makeSnapshotCore(builder,
                         makeMemoryPartitions(properties, 1)[0]);
        results.push_back(builder.build());
    }

    return results;
}

RegionPropertiesList findChangedRegionsRegionPoolMT(
    const std::vector<MemorySnapshot>& snaps1,
    const std::vector<MemorySnapshot>& snaps2, ThreadPool& tp,
    uintptr_t compareSize)
{
    auto vectorParts = makeVectorPartitionsFromSnapshotsList(
        snaps1, tp.numThreads());
    auto memoryParts = makeMemoryPartitionsFromSnapshotsList(snaps1);
    auto inputs = std::views::zip(memoryParts, snaps1, snaps2);
    auto consolidator =
        RegionPropertiesList::Consolidator(memoryParts.size());

    return executeParallelV1<RegionPropertiesList>(
        consolidator, tp, findChangedRegionsCore, vectorParts, inputs,
        compareSize);
}

std::vector<MemorySnapshot>
makeLotsOfSnapshotsMT(RegionPropertiesList& rl, ThreadPool& tp)
{
    std::vector<MemorySnapshot> results{};
    results.reserve(rl.size());

    std::vector<MemorySnapshot::Builder> builders;
    builders.reserve(rl.size());

    for (const auto& properties : rl)
    {
        builders.push_back(MemorySnapshot::Builder(properties));
    }

    std::vector<VectorPartition> parts =
        makeVectorPartitionsFromRegionPropertiesList(rl,
                                                     tp.numThreads());

    for (const auto& part : parts)
    {
        auto task = [part, &builders]()
        {
            // And then loop.
            for (size_t i = part.startIndex;
                 i < part.startIndex + part.size; i++)
            {
                makeSnapshotCore(builders[i],
                                 makeMemoryPartitions(
                                     builders[i].regionProperties, 1)
                                     .front());
            }
        };
        tp.submitTask(task);
    }

    tp.joinTasks();

    for (auto& builders : builders)
    {
        results.push_back(builders.build());
    }

    return results;
}
