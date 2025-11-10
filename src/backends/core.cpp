// Core functions go here.
// Core functions must follow the signature:
#include "data/maps.hpp"
#include "data/snapshots.hpp"
#include "backends/core.hpp"
#include "utils/logs.hpp"
#include <chrono>
#include <unistd.h>
#include <cstdint>
extern "C"
{
#include <bits/types/struct_iovec.h>
#include <sys/uio.h>
}

namespace rmf::backends::core
{

// A vector of chars, not snapshot because it'll be easier
// to combine them when needed?
// Consider the possibility of changing it.
MemorySnapshot makeSnapshotCore(const CoreInputs& core)
{
    if (!core.mrp)
    {
        rmf_Log(rmf_Error, "A MemeoryRegionProperties was not supplied!");
        return MemorySnapshot({}, {}, {});
    }
    const MemoryRegionProperties& mrp = core.mrp.value();
    rmf_Log(rmf_Debug, "Taking snapshot");
    using namespace std::chrono;

    struct iovec localIovec[1];
    struct iovec sourceIovec[1];

    rmf_Log(rmf_Debug,
        "Num bytes for this task: " << std::showbase << std::hex
                                    << mrp.relativeRegionSize);
    rmf_Log(rmf_Debug,
        "Actual Start address: " << std::showbase << std::hex
                                 << mrp.getActualRegionStart());
    rmf_Log(rmf_Debug,
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
                rmf_Log(rmf_Error,
                    "Completely failed to read the region. Error "
                    "is below.");
                perror("process_vm_readv");
                return MemorySnapshot({}, {}, {});
            }
            rmf_Log(rmf_Error,
                "Read " << nread << "/" << mrp.relativeRegionSize
                        << "bytes. Failed to read all the bytes "
                           "from that region.");

            return MemorySnapshot({}, {}, {});
        }
        totalBytesRead += nread;
    }

    return MemorySnapshot(data, mrp, steady_clock::now().time_since_epoch());
}

std::vector<MemoryRegionProperties>
findChangedRegionsCore(const CoreInputs& core,
                       const uintptr_t&  cmpSize)
{
    // TODO: requirement of core.mrp is redundant, as the snapshots
    // now supply that metadata.
    if (!core.snap1 || !core.snap2 || !core.mrp)
    {
        rmf_Log(rmf_Error, "Missing a snapshot!");
        return {};
    }
    const auto&                   snap1    = core.snap1.value();
    const auto&                   snap2    = core.snap2.value();
    const MemoryRegionProperties& mrp      = core.mrp.value();
    size_t                        numIters = snap1.size() / cmpSize + 1;
    uintptr_t                     current  = 0;
    rmf_Log(rmf_Debug, "Number of iterations required: " << numIters);

    std::vector<MemoryRegionProperties> data;
    for (size_t i = 0; i < numIters; ++i)
    {
        size_t actualCmpSize = (i == numIters - 1) ? snap1.size() - current : cmpSize;
        if (memcmp(snap1.data() + current, snap2.data() + current,
                   actualCmpSize))
        {
            rmf_Log(rmf_Message, "Found a change!");
            if (data.size() > 0 &&
                data.back().relativeRegionStart +
                        data.back().relativeRegionSize ==
                    mrp.relativeRegionStart + current)
            {
                data.back().relativeRegionSize += actualCmpSize;
            }
            else
            {
                MemoryRegionProperties nmrp = mrp;
                nmrp.relativeRegionSize     = actualCmpSize;
                nmrp.relativeRegionStart += current;
                data.push_back(std::move(nmrp));
            }
        }
        current += actualCmpSize;
    }
    rmf_Log(rmf_Debug, "Found changes: " << data.size());
    return data;
}

std::vector<MemoryRegionProperties>
findStringCore(const CoreInputs& core, const std::string_view& str)
{
    if (!core.snap1)
    {
        rmf_Log(rmf_Warning, "Snap1 was NOT provided!");
        return {};
    }
    std::vector<MemoryRegionProperties> result;
    const auto&                         snap1 = core.snap1.value();
    for (uintptr_t i = 0; i + str.length() <= snap1.size(); i++)
    {
        size_t count = 0;
        for (uintptr_t j = 0; j < str.length(); j++)
        {
            if (snap1[i + j] != str[j])
            {
                break;
            }
            count++;
        }
        if (count == str.length())
        {
            MemoryRegionProperties newRegion = snap1.regionProperties;
            // The cause of all my problems:
            // forgetting to add the offset because of the span...
            newRegion.relativeRegionStart =
                i + snap1.regionProperties.relativeRegionStart;
            newRegion.relativeRegionSize = str.length();
            result.push_back(newRegion);
        }
    }

    return result;
}

std::vector<MemoryRegionProperties>
findUnchangedRegionsCore(const CoreInputs& core, const uintptr_t& cmpSize)
{
    // TODO: requirement of core.mrp is redundant, as the snapshots
    // now supply that metadata.
    if (!core.snap1 || !core.snap2 || !core.mrp)
    {
        rmf_Log(rmf_Error, "Missing a snapshot!");
        return {};
    }
    const auto&                   snap1    = core.snap1.value();
    const auto&                   snap2    = core.snap2.value();
    const MemoryRegionProperties& mrp      = core.mrp.value();
    size_t                        numIters = snap1.size() / cmpSize + 1;
    uintptr_t                     current  = 0;
    rmf_Log(rmf_Debug, "Number of iterations required: " << numIters);

    std::vector<MemoryRegionProperties> data;
    for (size_t i = 0; i < numIters; ++i)
    {
        size_t actualCmpSize = (i == numIters - 1) ? snap1.size() - current : cmpSize;
        if (!memcmp(snap1.data() + current, snap2.data() + current,
                   actualCmpSize))
        {
            rmf_Log(rmf_Message, "Found no change!");
            if (data.size() > 0 &&
                data.back().relativeRegionStart +
                        data.back().relativeRegionSize ==
                    mrp.relativeRegionStart + current)
            {
                data.back().relativeRegionSize += actualCmpSize;
            }
            else
            {
                MemoryRegionProperties nmrp = mrp;
                nmrp.relativeRegionSize     = actualCmpSize;
                nmrp.relativeRegionStart += current;
                data.push_back(std::move(nmrp));
            }
        }
        current += actualCmpSize;
    }
    rmf_Log(rmf_Debug, "Found changes: " << data.size());
    return data;
}
};
