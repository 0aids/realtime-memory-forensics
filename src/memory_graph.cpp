#include "memory_graph.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <optional>

namespace rmf::graph
{

    std::string MemoryLink::toString() const
    {
        return std::format("id: {} policy: {} SourceAddress: "
                           "{:#x} TargetAddress: {:#x} name: {}",
                           id, magic_enum::enum_name(data.policy),
                           data.sourceAddr, data.targetAddr,
                           data.name);
    }

    std::string MemoryRegion::toString() const
    {
        std::string result =
            std::format("name: {} id: {}\ncomment: {}", data.name, id,
                        data.comment);

        for (const auto& namedValue : data.namedValues)
        {
            result += std::format("\n\tNamed value: {} offset: "
                                  "{:#x}\n\tcomment: {}",
                                  namedValue.name, namedValue.offset,
                                  namedValue.comment);
        }
        return result;
    }
    MemoryRegionID MemoryGraph::RegionAdd(MemoryRegionData data)
    {
        MemoryRegionID assignedID = m_nextValidRegionID++;
        m_regions.emplace(assignedID,
                          MemoryRegion{data, this, assignedID});
        return assignedID;
    }

    void MemoryGraph::RegionDelete(MemoryRegionID id)
    {
        m_regions.erase(id);
    }

    std::optional<MemoryRegion*>
    MemoryGraph::RegionGetFromID(MemoryRegionID id)
    {
        if (m_regions.contains(id))
            return std::optional<MemoryRegion*>{&m_regions.at(id)};
        return std::optional<MemoryRegion*>{std::nullopt};
    }

    MemoryLinkID MemoryGraph::LinkNaiveAdd(MemoryLinkData linkData)
    {
        MemoryLinkID assignedID = m_nextValidLinkID++;
        m_links.emplace(assignedID,
                        MemoryLink{linkData, this, assignedID});
        return assignedID;
    }

    void MemoryGraph::LinkDelete(MemoryLinkID id)
    {
        m_links.erase(id);
    }

    std::optional<MemoryLink*>
    MemoryGraph::LinkGetFromID(MemoryLinkID id)
    {
        if (m_links.contains(id))
            return std::optional<MemoryLink*>{&m_links.at(id)};
        return std::optional<MemoryLink*>{std::nullopt};
    }

    MemoryRegionID
    MemoryGraph::RegionGetRegionIdAtAddress(uintptr_t addr)
    {
        // Replace for binary search when using properly ordered region storage.
        for (const auto& region : RegionsGetViews())
        {
            if (region.data.mrp.TrueAddress() == addr)
                return region.id;
        }
        return noID_ce;
    }

    std::optional<MemoryRegion*>
    MemoryGraph::RegionGetRegionAtAddress(uintptr_t addr)
    {
        for (auto& region : RegionsGetViews())
        {
            if (region.data.mrp.TrueAddress() == addr)
                return std::optional<MemoryRegion*>{&region};
        }
        return std::optional<MemoryRegion*>{std::nullopt};
    }

    MemoryRegionID
    MemoryGraph::RegionGetRegionIdContainingAddress(uintptr_t addr)
    {
        for (const auto& region : RegionsGetViews())
        {
            if (region.data.mrp.TrueAddress() <= addr &&
                region.data.mrp.TrueEnd() > addr)
                return region.id;
        }
        return noID_ce;
    }

    std::optional<MemoryRegion*>
    MemoryGraph::RegionGetRegionContainingAddress(uintptr_t addr)
    {
        for (auto& region : RegionsGetViews())
        {
            if (region.data.mrp.TrueAddress() <= addr &&
                region.data.mrp.TrueEnd() > addr)
                return std::optional<MemoryRegion*>{&region};
        }
        return std::optional<MemoryRegion*>{std::nullopt};
    }

    MemoryRegionID
    MemoryGraph::_LinkCreateOrGetSource(uintptr_t sourceAddr)
    {
        MemoryRegionData::NamedValue linkingValue = {
            .name    = "source ptr",
            .type    = {types::typeName::_ptr},
            .offset  = 0,
            .comment = "",
        };
        auto region_o = RegionGetRegionContainingAddress(sourceAddr);
        if (region_o.has_value())
        {
            auto region_p = region_o.value();
            linkingValue.offset =
                sourceAddr - region_p->data.mrp.TrueAddress();
            region_p->data.namedValues.push_back(linkingValue);
            return region_o.value()->id;
        }
        else
            return RegionAdd({
                .mrp =
                    {
                          .parentRegionAddress   = sourceAddr,
                          .parentRegionSize      = 32,
                          .relativeRegionAddress = 0,
                          .relativeRegionSize    = 32,
                          .perms                 = types::Perms::Read,
                          .pid                   = 0,
                          },
                .namedValues = {linkingValue},
                .name        = "Source Region",
                .comment     = "Created from linking policy",
            });
    }

    MemoryRegionID
    MemoryGraph::_LinkCreateOrGetTarget(uintptr_t targetAddr)
    {
        auto region_o = RegionGetRegionContainingAddress(targetAddr);
        if (region_o.has_value())
        {
            return region_o.value()->id;
        }
        return RegionAdd({
            .mrp =
                {
                      .parentRegionAddress   = targetAddr,
                      .parentRegionSize      = 32,
                      .relativeRegionAddress = 0,
                      .relativeRegionSize    = 32,
                      .perms                 = types::Perms::Read,
                      .pid                   = 0,
                      },
            .namedValues = {},
            .name        = "Target Region",
            .comment     = "Created from linking policy",
        });
    }

    MemoryLinkID MemoryGraph::LinkSmartAdd(MemoryLinkData data)
    {
        // Loop through the unordered map to figure out if our link
        // lies anywhere.
        MemoryRegionID sourceID = noID_ce;
        MemoryRegionID targetID = noID_ce;
        switch (data.policy)
        {
            case MemoryLinkPolicy::Strict:
                sourceID = RegionGetRegionIdContainingAddress(
                    data.sourceAddr);
                targetID = RegionGetRegionIdContainingAddress(
                    data.targetAddr);
                break;
            case MemoryLinkPolicy::CreateSource:
                sourceID = _LinkCreateOrGetSource(data.sourceAddr);
                targetID = RegionGetRegionIdContainingAddress(
                    data.targetAddr);
                break;
            case MemoryLinkPolicy::CreateSourceTarget:
                sourceID = _LinkCreateOrGetSource(data.sourceAddr);
                targetID = _LinkCreateOrGetTarget(data.targetAddr);
                break;
            case MemoryLinkPolicy::CreateTarget:
                sourceID = RegionGetRegionIdContainingAddress(
                    data.sourceAddr);
                targetID = _LinkCreateOrGetTarget(data.targetAddr);
                break;
            default:
                rmf_Log(rmf_Error,
                        "Invalid policy chosen for smart creation!");
                break;
        }
        data.sourceID = sourceID;
        data.targetID = targetID;
        return LinkNaiveAdd(data);
    }

    void MemoryGraph::RegionsAssignToMaps(
        const types::MemoryRegionPropertiesVec& OGMaps)
    {
        for (auto& region : RegionsGetViews())
        {
            auto container_opt = OGMaps.GetRegionContainingAddress(
                region.data.mrp.TrueAddress());
            if (!container_opt.has_value())
            {
                rmf_Log(rmf_Warning,
                        "Region: '"
                            << region.toString()
                            << "' does not have containing region!");
                continue;
            }
            auto container = container_opt.value();
            region.data.mrp.AssignNewParentRegion(container);
        }
    }

    void MemoryGraph::RegionsAddMrpVec(
        const types::MemoryRegionPropertiesVec& mrpVec, std::string name)
    {
        size_t i = 0;
        for (const auto& region : mrpVec)
        {
            RegionAdd({
                .mrp         = region,
                .namedValues = {},
                .name        = name + std::to_string(i++),
                .comment     = "",
            });
        }
    }
}
