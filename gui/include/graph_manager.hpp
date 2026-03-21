#ifndef GRAPH_MANAGER_HPP
#define GRAPH_MANAGER_HPP

#include <imgui.h>
#include "memory_graph_viewer.hpp"
#include "types.hpp"

namespace rmf::gui
{

    class GraphManager
    {
      public:
        GraphManager();
        ~GraphManager() = default;

        // Add regions to the graph
        void addRegionFromProcess(
            const types::MemoryRegionProperties& mrp);
        void
        addRegionFromScan(const types::MemoryRegionProperties& mrp,
                          const std::string&                   name);

        // UI
        void draw();

      private:
        graph::MemoryGraphViewer m_viewer;
    };

}

#endif // GRAPH_MANAGER_HPP
