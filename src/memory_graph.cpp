#include "memory_graph.hpp"

namespace rmf::graph
{

    std::string MemoryRegionLinkData::toString() const
    {
        return std::format("id: {} policy: {} SourceAddress: "
                           "{:#x} TargetAddress: {:#x} name: {}",
                           id, magic_enum::enum_name(policy),
                           sourceAddr, targetAddr, name);
    }

    std::string MemoryRegionData::toString() const
    {
        std::string result = std::format(
            "name: {} id: {}\ncomment: {}", name, id, comment);

        for (const auto& namedValue : namedValues)
        {
            result += std::format("\n\tNamed value: {} offset: "
                                  "{:#x}\n\tcomment: {}",
                                  namedValue.name, namedValue.offset,
                                  namedValue.comment);
        }
        return result;
    }
    MemoryRegionID
    MemoryGraph::AddRegion(MemoryRegionData data)
    {
        MemoryRegionID assignedID = m_nextValidRegionID++;
        data.id = assignedID;
        m_regions.emplace(assignedID, MemoryRegion{data});
        return assignedID;
    }

    void MemoryGraph::RegionDelete(MemoryRegionID id)
    {
        m_regions.erase(id);
    }

    std::optional<MemoryRegion*> MemoryGraph::RegionGetFromID(MemoryRegionID id)
    {
        if (m_regions.contains(id))
            return std::optional<MemoryRegion*>{&m_regions.at(id)};
        return std::optional<MemoryRegion*>{std::nullopt};
    }

}
