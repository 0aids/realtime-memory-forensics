#include "types.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include <chrono>
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
    MemorySnapshot::Make(const MemoryRegionProperties& mrp)
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

            ssize_t nread = process_vm_readv(mrp.pid, localIovec, 1,
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
    MemoryRegionPropertiesVec::FilterPerms(
        const std::string_view& perms)
    {
        return utils::FilterPerms(*this, perms);
    }
    rmf::types::MemoryRegionPropertiesVec
    MemoryRegionPropertiesVec::FilterHasPerms(
        const std::string_view& perms)
    {
        return utils::FilterHasPerms(*this, perms);
    }
    rmf::types::MemoryRegionPropertiesVec
    MemoryRegionPropertiesVec::FilterNotPerms(
        const std::string_view& perms)
    {
        return utils::FilterNotPerms(*this, perms);
    }

    void MemoryRegion::AddNamedValue(const std::string& name,
                                     ptrdiff_t          offset,
                                     const std::string& comment)
    {
        rmf_Log(rmf_Verbose, "Adding named value: '" << name << "'");
        this->namedValues.emplace_back(name, offset, comment);
    }

    void MemoryRegion::RemoveNamedValue(const std::string& string)
    {
        for (size_t i = 0; i < this->namedValues.size(); i++)
        {
            if (namedValues[i].name == string)
            {
                this->namedValues.erase(this->namedValues.begin() +
                                        i);
                return;
            }
        }
        rmf_Log(rmf_Warning,
                "No such value of name '"
                    << string << "' exists in the named values");
        return;
    }


    const MemoryRegion&     MemoryGraph::GetMemoryRegionFromID(MemoryRegionID id) const
    {
        return m_memoryRegionMap.at(id);
    }
    const MemoryRegionLink& MemoryGraph::GetMemoryRegionLinkFromID(MemoryLinkID id) const
    {
        return m_memoryLinkMap.at(id);
    }
    size_t MemoryGraph::GetMemoryRegionMapSize() const
    {
        return m_memoryRegionMap.size();
    }
    size_t MemoryGraph::GetMemoryRegionLinkMapSize() const
    {
        return m_memoryLinkMap.size();
    }

    void RefreshableSnapshot::refresh()
    {
        rmf_Log(rmf_Error, "Unimplemented!");
    }

    void
    RefreshableSnapshot::refresh(const MemoryRegionProperties& mrp)
    {
        rmf_Log(rmf_Error, "Unimplemented!");
    }

    void MemoryRegion::RefreshSnapshot()
    {
        rmf_Log(rmf_Error, "Unimplemented!");
        // this->rsnap.refresh(this->mrp);
    }

    MemoryRegionProperties MemoryRegion::GetSnapshotMrp()
    {
        return this->rsnap.mrpModdable;
    }

    // void MemoryGraph::AddMemoryRegion(const MemoryRegion& region)
    // {
    //     auto id           = m_counterMemoryRegionID++;
    //     auto [_, success] = m_memoryRegionMap.try_emplace(
    //         id, region.mrp, this, id, region.namedValues, region.name,
    //         region.comment);
    //     if (!success)
    //     {
    //         rmf_Log(rmf_Error,
    //                 "Failed to emplace new region, ID already "
    //                 "exists????");
    //         return;
    //     }

    //     auto       head     = m_memoryRegionSpatialIndex.begin();
    //     const auto trueAddr = region.mrp.TrueAddress();
    //     // Empty spatial index
    //     if (head == m_memoryRegionSpatialIndex.end())
    //     {
    //         m_memoryRegionSpatialIndex.push_back(
    //             {id, region.mrp.TrueAddress(), region.mrp.TrueEnd()});
    //     }
    //     else
    //     {
    //         auto tail = head++;
    //         while (head != m_memoryRegionSpatialIndex.end())
    //         {
    //             // Insert between objects if possible.
    //             if (tail->start <= trueAddr && trueAddr < head->start)
    //             {
    //                 m_memoryRegionSpatialIndex.insert(
    //                     tail,
    //                     {id, region.mrp.TrueAddress(),
    //                      region.mrp.TrueEnd()});
    //                 return;
    //             }
    //             tail = head++;
    //         }
    //     }
    //     // either there was less than 2 objects, or this was the largest.
    //     m_memoryRegionSpatialIndex.push_back(
    //         {id, region.mrp.TrueAddress(), region.mrp.TrueEnd()});
    // }

    // void MemoryGraph::AddMemoryLink(const MemoryRegionLink& link)
    // {
    //     auto id           = m_counterMemoryLinkID++;
    //     auto [_, success] = m_memoryLinkMap.try_emplace(
    //         id, this, id, link.policy, link.sourceAddr,
    //         link.targetAddr);
    //     if (!success)
    //     {
    //         rmf_Log(rmf_Error,
    //                 "Failed to emplace new region, ID already "
    //                 "exists????");
    //         return;
    //     }
    //     // TODO: Update links automatically?
    // }

    // void MemoryGraph::RemoveMemoryRegion(MemoryRegionID id)
    // {
    //     if (!m_memoryRegionMap.erase(id))
    //     {
    //         rmf_Log(rmf_Error,
    //                 "Failed to remove memory region, ID: "
    //                     << id << " does not exist.");
    //     }
    //     // TODO: Remove irrelevant links.
    // }

    // void MemoryGraph::RemoveMemoryLink(MemoryLinkID id)
    // {
    //     if (!m_memoryLinkMap.erase(id))
    //     {
    //         rmf_Log(rmf_Error,
    //                 "Failed to remove memory link, ID: "
    //                     << id << " does not exist.");
    //     }
    // }
    void MemoryGraph::_CreateMemoryRegion(
        transaction::CreateMemoryRegion t)
    {
        t.parentGraph = this;
        m_memoryRegionMap.emplace(t.id, t);
        rmf_Log(rmf_Verbose,
                "Added a new memory region at ID: " << t.id);
    }
    void MemoryGraph::_UpdateMemoryRegion(
        transaction::UpdateMemoryRegion t)
    {
        m_memoryRegionMap.emplace(t.id, t);
        rmf_Log(rmf_Verbose, "Updated region at ID: " << t.id);
    }
    void MemoryGraph::_DeleteMemoryRegion(
        transaction::DeleteMemoryRegion t)
    {
        m_memoryRegionMap.erase(t.id);
        rmf_Log(rmf_Verbose, "Removed region at ID: " << t.id);
    }
    MemoryRegionLinkData
    MemoryGraph::_GetLinkDataFromName(const std::string& name)
    {
        // Search for the name, and copy the contents.
        // Most lazy version possible, will swap to using a map with string to id aliases.
        for (auto [id, link] : m_memoryLinkMap)
        {
            if (link.name == name)
            {
                // Returns a copy
                return link;
            }
        }
        rmf_Log(rmf_Error,
                "No such value with name: " << name << "exists!");
        return {};
    }

    // Here are the fucked ones.
    // How should we deal with links?
    // For now we will not cache links' destination MRs, and instead will
    // query them each time information is needed. Should be fine if we're handling
    // less than 10k links (unlikely)
    // For undo and redo, keep a stack of vectors of deltas.
    // But for now no undo and redo.
    void
    MemoryGraph::_CreateMemoryLink(transaction::CreateMemoryLink t)
    {
        constexpr uintptr_t d_defaultSourceMrSize = 8;
        constexpr uintptr_t d_defaultTargetMrSize = 8;
        t.parentGraph = this;
        // Do some fucked shit here too
        auto addMR = [this](auto& map, uint64_t id, uintptr_t addr)
        // TODO: Use m_parentRegions to resolve the parent regions.
        // For now we'll just do some funcky ahh stuff
        {
            MemoryRegionProperties mrp = {addr, d_defaultSourceMrSize,
                                          0, d_defaultSourceMrSize};
            MemoryRegion           mr  = {
                {.mrp=mrp, .parentGraph=this, .id=id, .name="autogenerated"}
            };
            map.emplace(id, mr);
        };
        // Find the source memory region
        MemoryRegionID srcID =
            FindMemoryRegionContainingAddress(t.sourceAddr);
        // Find the target memory region
        MemoryRegionID targetID =
            FindMemoryRegionContainingAddress(t.targetAddr);
        switch (t.policy)
        {
            case MemoryLinkPolicy::Strict:
                rmf_Log(rmf_Verbose,
                        "Policy does not allow creation");
                return;
            case MemoryLinkPolicy::CreateSource:
                if (!targetID)
                {
                    rmf_Log(rmf_Verbose,
                            "Policy does not allow creation of "
                            "target. (create source only)");
                    return;
                }

                if (!srcID)
                {
                    rmf_Log(rmf_Verbose,
                            "Source doesn't exist, policy will "
                            "create it!");
                    srcID = m_counterMemoryRegionID++;
                    addMR(m_memoryRegionMap, srcID, t.sourceAddr);
                }
                break;
            case MemoryLinkPolicy::CreateTarget:
                if (!srcID)
                {
                    rmf_Log(rmf_Verbose,
                            "Policy does not allow creation. (create "
                            "target only)");
                    return;
                }
                if (!targetID)
                {
                    rmf_Log(rmf_Verbose,
                            "Target doesn't exist, policy will "
                            "create it!");
                    targetID = m_counterMemoryRegionID++;
                    addMR(m_memoryRegionMap, targetID, t.targetAddr);
                }
                break;
            case MemoryLinkPolicy::CreateSourceTarget:
                if (!srcID)
                {
                    rmf_Log(rmf_Verbose,
                            "Source doesn't exist, policy will "
                            "create it!");
                    srcID = m_counterMemoryRegionID++;
                    addMR(m_memoryRegionMap, srcID, t.sourceAddr);
                }
                if (!targetID)
                {
                    rmf_Log(rmf_Verbose,
                            "Target doesn't exist, policy will "
                            "create it!");
                    targetID = m_counterMemoryRegionID++;
                    addMR(m_memoryRegionMap, targetID, t.targetAddr);
                }
                break;
        }
        m_memoryRegionMap[srcID].outgoingLinkIDs.emplace(t.id);
        m_memoryRegionMap[targetID].incomingLinkIDs.emplace(t.id);
        m_memoryLinkMap.emplace(t.id,
                                static_cast<MemoryRegionLink>(t));
        // Update source and target with new links
        // Add regions or return
    }
    void
    MemoryGraph::_UpdateMemoryLink(transaction::UpdateMemoryLink t)
    {
        rmf_Log(rmf_Error, "Unimplemented!");
        // Find source
        // Find target
        // Satisfy policy
        // Update source and target
    }
    void
    MemoryGraph::_DeleteMemoryLink(transaction::DeleteMemoryLink t)
    {
        rmf_Log(rmf_Error, "Unimplemented!");
        // Find link
        // Find link's source
        // Find link's target
        // Unlink target and source
        // Delete link
    }

    void MemoryGraph::Clear()
    {
        rmf_Log(rmf_Warning, "Clearing all graph nodes!");
        m_memoryLinkMap.clear();
        m_memoryRegionMap.clear();
    }

    bool MemoryGraph::ProcessTransaction(
        transaction::TransactionType transaction)
    {
        using namespace transaction;
        return std::visit(
            [this](auto&& arg)
            {
                using type = std::decay_t<decltype(arg)>;
                if constexpr (std::same_as<type, CreateMemoryRegion>)
                {
                    _CreateMemoryRegion(arg);
                }
                else if constexpr (std::same_as<type,
                                                UpdateMemoryRegion>)
                {
                    _UpdateMemoryRegion(arg);
                }
                else if constexpr (std::same_as<type,
                                                DeleteMemoryRegion>)
                {
                    _DeleteMemoryRegion(arg);
                }
                else if constexpr (std::same_as<type,
                                                CreateMemoryLink>)
                {
                    _CreateMemoryLink(arg);
                }
                else if constexpr (std::same_as<type,
                                                UpdateMemoryLink>)
                {
                    _UpdateMemoryLink(arg);
                }
                else if constexpr (std::same_as<type,
                                                DeleteMemoryLink>)
                {
                    _DeleteMemoryLink(arg);
                }
                else
                {
                    // static_assert(false, "Invalid variant type for transaction!");
                    // Always false doesn't want to work because this is not a templated
                    // function.
                    rmf_Log(rmf_Error,
                            "idk how you did this but you inputted "
                            "an invalid variant type");
                    return false;
                }
                return false;
            },
            transaction);
    }
    MemoryRegionData
    MemoryGraph::_GetRegionDataFromName(const std::string& name)
    {
        // Search for the name, and copy the contents.
        // Most lazy version possible, will swap to using a map with string to id aliases.
        for (auto [id, mr] : m_memoryRegionMap)
        {
            if (mr.name == name)
            {
                return mr;
            }
        }
        rmf_Log(rmf_Error,
                "No such value with name: " << name << "exists!");
        return {};
    }
    MemoryRegionID
    MemoryGraph::FindMemoryRegionContainingAddress(uintptr_t addr)
    {
        for (auto& [id, mr] : m_memoryRegionMap)
        {
            if (mr.mrp.TrueAddress() <= addr &&
                addr < mr.mrp.TrueEnd())
                return id;
        }
        rmf_Log(rmf_Verbose,
                "No Memory Regions that hold "
                    << std::hex << std::showbase << addr);
        return 0;
    }
    transaction::CreateMemoryRegion
    MemoryGraph::StartCreateMemoryRegionTransaction()
    {
        // Bruh what? apparently because the value is inherited.
        return {{.id = m_counterMemoryRegionID++}};
    }
    transaction::UpdateMemoryRegion
    MemoryGraph::StartUpdateMemoryRegionTransaction(
        std::optional<std::string>    name,
        std::optional<MemoryRegionID> id)
    {
        if (name && id)
        {
            rmf_Log(rmf_Error,
                    "Name and ID was provided, defaulting to ID");
            name = std::nullopt;
        }
        if (name) {}
        else if (id)
        {
            if (m_memoryRegionMap.count(id.value()) == 1)
            {
                return static_cast<transaction::UpdateMemoryRegion>(
                    m_memoryRegionMap[id.value()]);
            }
            rmf_Log(rmf_Error,
                    "ID: " << id.value() << " does not exist!");
        }
        rmf_Log(rmf_Error,
                "Name or ID was not provided, cannot start "
                "MemoryUpdateTransaction");
        return {};
    }
    transaction::DeleteMemoryRegion
    MemoryGraph::StartDeleteMemoryRegionTransaction(
        std::optional<std::string>    name,
        std::optional<MemoryRegionID> id)
    {
        if (id && !m_memoryRegionMap.count(id.value()))
        {
            rmf_Log(rmf_Error,
                    "ID: " << id.value() << " Does not exist!");
            return {};
        }
        if (id)
            return {.id = id.value()};
        if (name)
        {
            auto link = _GetLinkDataFromName(name.value());
            return {.id = link.id};
        }
        rmf_Log(rmf_Error,
                "Name or ID was not provided, cannot start "
                "DeleteMemoryLink Transaction");
        return {};
    }

    transaction::CreateMemoryLink
    MemoryGraph::StartCreateMemoryLinkTransaction()
    {
        return {{.id = m_counterMemoryLinkID++}};
    }
    transaction::UpdateMemoryLink
    MemoryGraph::StartUpdateMemoryLinkTransaction(
        std::optional<std::string>  name,
        std::optional<MemoryLinkID> id)
    {
        if (name && id)
        {
            rmf_Log(rmf_Error,
                    "Name and ID was provided, defaulting to ID");
            name = std::nullopt;
        }
        if (name)
        {
            return {_GetLinkDataFromName(name.value())};
        }
        else if (id)
        {
            if (m_memoryLinkMap.count(id.value()) == 1)
            {
                return static_cast<transaction::UpdateMemoryLink>(
                    m_memoryLinkMap[id.value()]);
            }
            rmf_Log(rmf_Error,
                    "ID: " << id.value() << " does not exist!");
            return {};
        }
        else
        {
            rmf_Log(rmf_Error,
                    "Name or ID was not provided, cannot start "
                    "UpdateMemoryLink Transaction");
            return {};
        }
    }
    transaction::DeleteMemoryLink
    MemoryGraph::StartDeleteMemoryLinkTransaction(
        std::optional<std::string>  name,
        std::optional<MemoryLinkID> id)
    {
        if (id && !m_memoryLinkMap.count(id.value()))
        {
            rmf_Log(rmf_Error,
                    "ID: " << id.value() << " Does not exist!");
            return {};
        }
        if (id)
            return {.id = id.value()};
        if (name)
        {
            auto link = _GetLinkDataFromName(name.value());
            return {.id = link.id};
        }
        rmf_Log(rmf_Error,
                "Name or ID was not provided, cannot start "
                "DeleteMemoryLink Transaction");
        return {};
    }

    std::string
    MemoryRegion::toStringDetailed(bool showLinkDetails) const
    {
        std::string result = toString();
        result += "\n ****Outgoing Links****: ";
        for (MemoryLinkID id : outgoingLinkIDs)
        {
            result += std::format("\n**linkID: {}**", id);
            if (showLinkDetails)
            {
                auto details = parentGraph->GetMemoryRegionLinkFromID(id).toString();
                result += "\n" + details;
            }
        }
        result += "\n ****Incoming Links****: ";
        for (auto id : incomingLinkIDs)
        {
            result += std::format("\n**linkID: {}**", id);
            if (showLinkDetails)
            {
                auto details = parentGraph->GetMemoryRegionLinkFromID(id).toString();
                result += "\n" + details;
            }
        }
        return result;
    }

}
