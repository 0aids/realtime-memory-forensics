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
        // --- Node Editor State ---
        char      m_bufName[128]    = "New Region";
        char      m_bufComment[256] = "";
        uintptr_t m_inputAddr       = 0x0;
        uintptr_t m_inputSize       = 1024;
        uintptr_t m_inputParentAddr = 0x0;
        uintptr_t m_inputParentSize = 0;
        rmf::types::Perms m_inputPerms = rmf::types::Perms::None;
        int       m_inputPid        = 0;
        std::vector<rmf::graph::MemoryRegionData::NamedValue>
            m_tempNamedValues;

        // --- Link Editor State ---
        char m_linkNameBuf[128] = "New Link";
        uintptr_t m_linkSourceAddr = 0x0;
        uintptr_t m_linkTargetAddr = 0x0;
        rmf::graph::MemoryLinkPolicy m_linkPolicy = rmf::graph::MemoryLinkPolicy::Strict;
        rmf::graph::MemoryLinkID m_editingLinkID = -1;
        bool m_isEditingLink = false;

        // Track editing state
        bool                       m_isEditingMode = false;
        rmf::graph::MemoryRegionID m_editingID =
            -1; // -1 or generic invalid ID

        // --- Helper: Reset to "Create New" state ---
        void ResetRegionBuilderState();
        void ResetLinkBuilderState();

        // --- Helper: Load existing node data into inputs ---
        void
        LoadRegionIntoState(const rmf::graph::MemoryRegionData& data);
        void
        LoadLinkIntoState(const rmf::graph::MemoryRegionLinkData& data);

      private:
        // --- Drawing helpers ---
        void drawSidebar();
        void drawNodeEditorTab();
        void drawLinkEditorTab();
        void drawEditor();
        void drawNode(const rmf::graph::MemoryRegion& node);
        void handlePostFrameLogic(bool deletePressed);

      public:
        std::shared_ptr<MemoryGraph> sp_mg =
            std::make_shared<MemoryGraph>();
        void draw();
    };
};

#endif // memory_graph_viewer_hpp_INCLUDED
