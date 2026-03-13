#include "graph_manager.hpp"

namespace rmf::gui
{

    GraphManager::GraphManager() {}

    void GraphManager::addRegionFromProcess(
        const types::MemoryRegionProperties& mrp)
    {
        graph::MemoryRegionData newData;
        newData.name = *mrp.regionName_sp;
        newData.mrp  = mrp;
        m_viewer.sp_mg->RegionAdd(newData);
    }

    void GraphManager::addRegionFromScan(
        const types::MemoryRegionProperties& mrp,
        const std::string&                   name)
    {
        graph::MemoryRegionData newData;
        newData.name = name;
        newData.mrp  = mrp;
        m_viewer.sp_mg->RegionAdd(newData);
    }

    void GraphManager::draw()
    {
        if (ImGui::Begin("Node Viewer"))
        {
            m_viewer.draw();
        }
        ImGui::End();
    }

}
