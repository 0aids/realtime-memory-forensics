#ifndef memory_graph_viewer_hpp_INCLUDED
#define memory_graph_viewer_hpp_INCLUDED

#include <imgui.h>
#include <imnodes.h>
#include "memory_graph.hpp"
#include <memory>

namespace rmf::graph
{
    class MemoryGraphViewer
    {
        // Member, unique ptr
        // --- State Variables ---
        // Ideally these should be members of MemoryGraphViewer
        char      m_bufName[128]    = "New Region";
        char      m_bufComment[256] = "";
        uintptr_t m_inputAddr       = 0x0;
        uintptr_t m_inputSize       = 1024;
        int       m_inputPid        = 0;
        std::vector<rmf::graph::MemoryRegionData::NamedValue>
            m_tempNamedValues;

        // Track editing state
        bool                       m_isEditingMode = false;
        rmf::graph::MemoryRegionID m_editingID =
            -1; // -1 or generic invalid ID

        // --- Helper: Reset to "Create New" state ---
        void ResetRegionBuilderState();

        // --- Helper: Load existing node data into inputs ---
        void
        LoadRegionIntoState(const rmf::graph::MemoryRegionData& data);

      public:
        std::shared_ptr<MemoryGraph> sp_mg =
            std::make_shared<MemoryGraph>();
        void draw();
    };
};

#endif // memory_graph_viewer_hpp_INCLUDED
