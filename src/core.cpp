// Core functions go here.
// Core functions must follow the signature:
// XCore(makeSnapshot)
#include "maps.hpp"
#include "snapshots.hpp"
#include "core.hpp"
#include "logs.hpp"
#include <unistd.h>
#include <cstdint>
extern "C"
{
#include <bits/types/struct_iovec.h>
#include <sys/uio.h>
}

std::vector<char> makeSnapshotCore(const CoreInputs& core)
{
    if (!core.mrp)
    {
        Log(Error, "A MemeoryRegionProperties was not supplied!");
        return {};
    }
    const MemoryRegionProperties& mrp = core.mrp.value();
    Log(Debug, "Taking snapshot");
    using namespace std::chrono;

    struct iovec localIovec[1];
    struct iovec sourceIovec[1];

    Log(Debug,
        "Num bytes for this task: " << std::showbase << std::hex
                                    << mrp.relativeRegionSize);
    Log(Debug,
        "Actual Start address: " << std::showbase << std::hex
                                 << mrp.getActualRegionStart());
    Log(Debug,
        "Size: " << std::showbase << std::hex
                 << mrp.relativeRegionSize);

    const size_t      chunks = 1 << 24;

    int64_t           totalBytesRead = 0;

    std::vector<char> data;
    data.resize(mrp.relativeRegionSize);

    while (totalBytesRead <
           static_cast<int64_t>(mrp.relativeRegionSize))
    {
        uint64_t bytesToRead =
            (mrp.relativeRegionSize - totalBytesRead >
             (ssize_t)chunks) ?
            chunks :
            mrp.relativeRegionSize - totalBytesRead;
        sourceIovec[0].iov_base =
            (void*)(mrp.getActualRegionStart() + totalBytesRead);
        sourceIovec[0].iov_len = bytesToRead;

        localIovec[0].iov_base = data.data() + totalBytesRead;
        localIovec[0].iov_len  = bytesToRead;

        // This is a lot simpler than using lseek, and permissions are granted (no
        // sudo) if yama is 0
        ssize_t nread = process_vm_readv(mrp.pid, localIovec, 1,
                                         sourceIovec, 1, 0);
        if (nread <= 0)
        {
            if (nread == -1)
            {
                Log(Error,
                    "Completely failed to read the region. Error "
                    "is below.");
                perror("process_vm_readv");
                return {};
            }
            Log(Error,
                "Read " << nread << "/" << mrp.relativeRegionSize
                        << "bytes. Failed to read all the bytes "
                           "from that region.");

            return {};
        }
        totalBytesRead += nread;
    }

    return std::move(data);
}

std::vector<MemoryRegionProperties>
findChangedRegionsCore(const CoreInputs& core,
                       const uintptr_t &cmpSize)
{
    if (!core.snap1 || !core.snap2 || !core.mrp) {
        Log(Error, "Missing a snapshot!");
        return {};
    }
    const auto &snap1 = core.snap1.value();
    const auto &snap2 = core.snap2.value();
    const MemoryRegionProperties &mrp = core.mrp.value();
    size_t    numIters = snap1.size() / cmpSize;
    uintptr_t current  = 0;
    Log(Debug, "Number of iterations required: " << numIters);

    std::vector<MemoryRegionProperties> data;
    for (size_t i = 0; i < numIters; ++i)
    {
        uintptr_t curSize = (i == numIters - 1) ?
            snap1.size() - current :
            cmpSize;

        if (memcmp(snap1.data() + current, snap2.data() + current,
                   curSize))
        {
            Log(Message, "Found a change!");
            if (data.size() > 0 &&
                data.back().relativeRegionStart +
                        data.back().relativeRegionSize ==
                    current)
            {
                data.back().relativeRegionSize += curSize;
            }
            else
            {
                MemoryRegionProperties nmrp = mrp;
                nmrp.relativeRegionSize     = curSize;
                nmrp.relativeRegionStart    = current;
                data.push_back(std::move(nmrp));
            }
        }
        current += curSize;
    }
    Log(Debug, "Found changes: " << data.size());
    return std::move(data);
}
