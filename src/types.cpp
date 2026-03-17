#include "types.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <optional>
#include <sstream>
#include <unistd.h>
#include <cstdint>
#include <unordered_map>
#include <variant>
extern "C"
{
#include <bits/types/struct_iovec.h>
#include <sys/uio.h>
}

namespace rmf::types
{
    MemorySnapshot::MemorySnapshot(const MemoryRegionProperties& _mrp)
    {
        d = std::make_shared<Data>(_mrp);
    }

    MemorySnapshot
    MemorySnapshot::Make(const MemoryRegionProperties& mrp, pid_t pid)
    {
        constexpr uintptr_t chunkSize = 1 << 24;
        MemorySnapshot      snap(mrp);

        rmf_Log(rmf_Debug, "Taking snapshot!");
        auto before =
            std::chrono::steady_clock::now().time_since_epoch();

        struct iovec localIovec[1];
        struct iovec sourceIovec[1];

        snap.d->mc_data.resize(mrp.relativeRegionSize);
        intptr_t totalBytesRead = 0;
        while (totalBytesRead <
               static_cast<intptr_t>(mrp.relativeRegionSize))
        {
            uintptr_t bytesToRead =
                (mrp.relativeRegionSize - totalBytesRead >
                 chunkSize) ?
                chunkSize :
                mrp.relativeRegionSize - totalBytesRead;

            sourceIovec[0].iov_base =
                (void*)(mrp.TrueAddress() + totalBytesRead);
            sourceIovec[0].iov_len = bytesToRead;

            localIovec[0].iov_base =
                snap.d->mc_data.data() + totalBytesRead;
            localIovec[0].iov_len = bytesToRead;

            ssize_t nread = process_vm_readv(pid, localIovec, 1,
                                             sourceIovec, 1, 0);

            if (nread <= 0)
            {
                if (nread == -1 && totalBytesRead > 0)
                {
                    snap.d->mc_data.clear();
                    rmf_Log(
                        rmf_Error,
                        "Read "
                            << nread << "/" << mrp.relativeRegionSize
                            << "bytes. Failed to read all the bytes "
                               "from that region.");
                    rmf_Log(rmf_Error, "Error is below: ");
                    perror("process_vm_readv");
                    return snap;
                }
                rmf_Log(rmf_Error,
                        "Completely failed to read the region. "
                        "Error is below");
                perror("process_vm_readv");
                snap.d->mc_data.resize(totalBytesRead);
                return snap;
            }
            totalBytesRead += nread;
        }

        auto after =
            std::chrono::steady_clock::now().time_since_epoch();

        rmf_Log(
            rmf_Debug,
            "Time taken for snapshot: " << std::chrono::duration_cast<
                std::chrono::milliseconds>(after - before));

        return snap;
    }

    void MemorySnapshot::printHex(size_t charsPerLine,
                                  size_t numLines) const
    {
        std::stringstream ss;
        if (charsPerLine == 0)
            charsPerLine = 32;
        if (numLines == 0)
            numLines = SIZE_MAX;
        ss << std::format("{:#0x}: ", d->mrp.TrueAddress());
        for (size_t i = 0; i < d->mc_data.size(); i++)
        {
            ss << std::format("{:02x} ", d->mc_data[i]);
            i++;
            if (i % 8 == 0 && i % charsPerLine > 0 && i > 0)
                ss << "  ";
            if (i % charsPerLine == 0 && i > 0)
            {
                numLines--;
                if (numLines == 0 || i >= d->mc_data.size())
                    break;
                ss << "\n";
                ss << std::format("{:#0x}: ",
                                  d->mrp.TrueAddress() + i);
            }
            i--;
        }
        std::cout << ss.str() << std::endl;
    }
    rmf::types::MemoryRegionPropertiesVec
    MemoryRegionProperties::BreakIntoChunks(uintptr_t chunkSize,
                                            uintptr_t overlapSize)
    {
        return utils::BreakIntoChunks(*this, chunkSize, overlapSize);
    }

    void MemoryRegionProperties::AssignNewParentRegion(
        const MemoryRegionProperties& other)
    {
        // This is sus, write test cases please.
        // Calculate new relative region address.
        ptrdiff_t diff =
            parentRegionAddress - other.parentRegionAddress;
        relativeRegionAddress += diff;
        parentRegionAddress = other.parentRegionAddress;
        diff = relativeRegionSize - other.parentRegionSize;
        if (TrueEnd() >
            other.parentRegionAddress + other.parentRegionSize)
        {
            rmf_Log(rmf_Warning,
                    "Region: '"
                        << this->toString()
                        << "' has a parent region that is smaller "
                           "than itself! Will truncate!");
            // Truncate the relative region size
            relativeRegionSize -= diff;
        }
        parentRegionSize = other.parentRegionSize;
        regionName_sp    = other.regionName_sp;
        perms            = other.perms;
    }

    rmf::types::MemoryRegionPropertiesVec
    MemoryRegionPropertiesVec::BreakIntoChunks(uintptr_t chunkSize,
                                               uintptr_t overlapSize)
    {
        return utils::BreakIntoChunks(*this, chunkSize, overlapSize);
    }

    // Just pass on the filters to the already implemented versions.
    rmf::types::MemoryRegionPropertiesVec
    MemoryRegionPropertiesVec::FilterMaxSize(const uintptr_t maxSize)
    {
        return utils::FilterMaxSize(*this, maxSize);
    }
    rmf::types::MemoryRegionPropertiesVec
    MemoryRegionPropertiesVec::FilterMinSize(const uintptr_t minSize)
    {
        return utils::FilterMinSize(*this, minSize);
    }
    rmf::types::MemoryRegionPropertiesVec
    MemoryRegionPropertiesVec::FilterName(
        const std::string_view& string)
    {
        return utils::FilterName(*this, string);
    }
    rmf::types::MemoryRegionPropertiesVec
    MemoryRegionPropertiesVec::FilterContainsName(
        const std::string_view& string)
    {
        return utils::FilterContainsName(*this, string);
    }
    rmf::types::MemoryRegionPropertiesVec
    MemoryRegionPropertiesVec::FilterExactPerms(
        const std::string_view& perms)
    {
        return utils::FilterExactPerms(*this, perms);
    }
    rmf::types::MemoryRegionPropertiesVec
    MemoryRegionPropertiesVec::FilterHasPerms(
        const std::string_view& perms) const
    {
        return utils::FilterHasPerms(*this, perms);
    }
    rmf::types::MemoryRegionPropertiesVec
    MemoryRegionPropertiesVec::FilterNotPerms(
        const std::string_view& perms)
    {
        return utils::FilterNotPerms(*this, perms);
    }
    std::optional<MemoryRegionProperties>
    rmf::types::MemoryRegionPropertiesVec::GetRegionContainingAddress(
        uintptr_t addr) const
    {
        for (const auto& mrp : *this)
        {
            if (mrp.TrueAddress() <= addr && mrp.TrueEnd() > addr)
                return mrp;
        }
        return std::nullopt;
    }

    rmf::types::MemoryRegionPropertiesVec
    MemoryRegionPropertiesVec::FilterActiveRegions(pid_t pid)
    {
        return utils::FilterActiveRegions(*this, pid);
    }
    MemoryRegionPropertiesVec::MemoryRegionPropertiesVec(
        std::vector<MemoryRegionProperties>&& other) :
        std::vector<MemoryRegionProperties>(other)
    {
    }
    MemoryRegionPropertiesVec::MemoryRegionPropertiesVec(
        const std::vector<MemoryRegionProperties>& other) :
        std::vector<MemoryRegionProperties>(other)
    {
    }
}
