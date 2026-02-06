#include "memory_graph_viewer.hpp"
#include "imgui.h"
#include "imnodes.h"
#include "memory_graph.hpp"
#include <magic_enum/magic_enum.hpp>

namespace rmf::graph
{
    void MemoryGraphViewer::ResetRegionBuilderState()
    {
        m_isEditingMode = false;
        m_editingID     = -1;

        snprintf(m_bufName, sizeof(m_bufName), "New Region");
        m_bufComment[0]   = '\0';
        m_inputAddr       = 0x0;
        m_inputSize       = 1024;
        m_inputParentAddr = 0x0;
        m_inputParentSize = 0;
        m_inputPerms      = rmf::types::Perms::None;
        m_inputPid        = 0;
        m_tempNamedValues.clear();
    }

    void MemoryGraphViewer::ResetLinkBuilderState()
    {
        m_isEditingLink = false;
        m_editingLinkID = -1;
        snprintf(m_linkNameBuf, sizeof(m_linkNameBuf), "New Link");
        m_linkSourceAddr = 0x0;
        m_linkTargetAddr = 0x0;
        m_linkPolicy     = rmf::graph::MemoryLinkPolicy::Strict;
    }

    void MemoryGraphViewer::LoadRegionIntoState(
        const rmf::graph::MemoryRegionData& data)
    {
        m_isEditingMode = true;
        m_editingID     = data.id;

        strncpy(m_bufName, data.name.c_str(), sizeof(m_bufName) - 1);
        m_bufName[sizeof(m_bufName) - 1] = '\0';

        strncpy(m_bufComment, data.comment.c_str(),
                sizeof(m_bufComment) - 1);
        m_bufComment[sizeof(m_bufComment) - 1] = '\0';

        m_inputAddr       = data.mrp.relativeRegionAddress;
        m_inputSize       = data.mrp.relativeRegionSize;
        m_inputParentAddr = data.mrp.parentRegionAddress;
        m_inputParentSize = data.mrp.parentRegionSize;
        m_inputPerms      = data.mrp.perms;
        m_inputPid        = data.mrp.pid;
        m_tempNamedValues = data.namedValues;
    }

    void MemoryGraphViewer::LoadLinkIntoState(
        const rmf::graph::MemoryRegionLinkData& data)
    {
        m_isEditingLink = true;
        m_editingLinkID = data.id;

        strncpy(m_linkNameBuf, data.name.c_str(),
                sizeof(m_linkNameBuf) - 1);
        m_linkNameBuf[sizeof(m_linkNameBuf) - 1] = '\0';

        m_linkSourceAddr = data.sourceAddr;
        m_linkTargetAddr = data.targetAddr;
        m_linkPolicy     = data.policy;
    }

    void MemoryGraphViewer::drawNodeEditorTab()
    {
        using namespace magic_enum::bitwise_operators;
        // Header changes based on mode
        if (m_isEditingMode)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                               "Editing Region ID: %lu", m_editingID);
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                ResetRegionBuilderState();
                ImNodes::ClearNodeSelection();
            }
            auto region = sp_mg->RegionGetFromID(m_editingID);
            if (region.has_value())
            {
                ImGui::Text(
                    "Original Name: %s",
                    region.value()->data.mrp.regionName_sp->c_str());
            }
        }
        else
        {
            ImGui::Text("Create New Region");
        }

        ImGui::Separator();

        ImGui::InputText("Name", m_bufName, IM_ARRAYSIZE(m_bufName));
        ImGui::InputText("Comment", m_bufComment,
                         IM_ARRAYSIZE(m_bufComment));
        ImGui::InputInt("PID", &m_inputPid);

        ImGui::SeparatorText("Region Properties");
        ImGui::InputScalar("Parent Addr", ImGuiDataType_U64,
                           &m_inputParentAddr, NULL, NULL, "%016llX",
                           ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::InputScalar("Parent Size", ImGuiDataType_U64,
                           &m_inputParentSize);
        ImGui::InputScalar("Relative Addr", ImGuiDataType_U64,
                           &m_inputAddr, NULL, NULL, "%016llX",
                           ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::InputScalar("Relative Size", ImGuiDataType_U64,
                           &m_inputSize);

        ImGui::SeparatorText("Permissions");
        bool p_read = (m_inputPerms & rmf::types::Perms::Read) !=
            rmf::types::Perms::None;
        bool p_write = (m_inputPerms & rmf::types::Perms::Write) !=
            rmf::types::Perms::None;
        bool p_exec = (m_inputPerms & rmf::types::Perms::Execute) !=
            rmf::types::Perms::None;
        bool p_shared = (m_inputPerms & rmf::types::Perms::Shared) !=
            rmf::types::Perms::None;

        if (ImGui::Checkbox("Read", &p_read))
        {
            m_inputPerms = p_read ?
                (m_inputPerms | rmf::types::Perms::Read) :
                (m_inputPerms & ~rmf::types::Perms::Read);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Write", &p_write))
        {
            m_inputPerms = p_write ?
                (m_inputPerms | rmf::types::Perms::Write) :
                (m_inputPerms & ~rmf::types::Perms::Write);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Execute", &p_exec))
        {
            m_inputPerms = p_exec ?
                (m_inputPerms | rmf::types::Perms::Execute) :
                (m_inputPerms & ~rmf::types::Perms::Execute);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Shared", &p_shared))
        {
            m_inputPerms = p_shared ?
                (m_inputPerms | rmf::types::Perms::Shared) :
                (m_inputPerms & ~rmf::types::Perms::Shared);
        }

        ImGui::SeparatorText("Named Values");

        ImGui::BeginChild("VarsList", ImVec2(0, 150), true);
        if (ImGui::Button("+ Add Value"))
        {
            m_tempNamedValues.push_back(
                {"Var", rmf::types::type{rmf::types::typeName::_u64},
                 0x0, ""});
        }
        for (size_t i = 0; i < m_tempNamedValues.size(); ++i)
        {
            ImGui::PushID(i);
            char varNameBuf[64];
            strncpy(varNameBuf, m_tempNamedValues[i].name.c_str(),
                    sizeof(varNameBuf));
            ImGui::SetNextItemWidth(90);
            if (ImGui::InputText("##vn", varNameBuf,
                                 sizeof(varNameBuf)))
            {
                m_tempNamedValues[i].name = varNameBuf;
            }
            ImGui::SameLine();

            ImGui::SetNextItemWidth(70);
            auto type_names =
                magic_enum::enum_names<rmf::types::typeName>();
            int current_item =
                static_cast<int>(m_tempNamedValues[i].type.type);
            if (ImGui::BeginCombo(
                    "##type",
                    std::string(type_names[current_item]).c_str()))
            {
                for (size_t n = 0; n < type_names.size(); n++)
                {
                    bool is_selected =
                        (current_item == static_cast<int>(n));
                    if (ImGui::Selectable(
                            std::string(type_names[n]).c_str(),
                            is_selected))
                    {
                        m_tempNamedValues[i].type.type =
                            static_cast<rmf::types::typeName>(n);
                    }
                    if (is_selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(60);
            ImGui::InputScalar("##vo", ImGuiDataType_S64,
                               &m_tempNamedValues[i].offset, NULL,
                               NULL, "%X",
                               ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::SameLine();
            if (ImGui::Button("x"))
            {
                m_tempNamedValues.erase(m_tempNamedValues.begin() +
                                        i);
                ImGui::PopID();
                continue;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0, 10));

        if (m_isEditingMode)
        {
            if (ImGui::Button("Save Changes", ImVec2(-FLT_MIN, 0)))
            {
                auto optRegion = sp_mg->RegionGetFromID(m_editingID);
                if (optRegion.has_value())
                {
                    MemoryRegion* region = optRegion.value();
                    region->data.name    = m_bufName;
                    region->data.comment = m_bufComment;
                    region->data.mrp.relativeRegionAddress =
                        m_inputAddr;
                    region->data.mrp.relativeRegionSize = m_inputSize;
                    region->data.mrp.parentRegionAddress =
                        m_inputParentAddr;
                    region->data.mrp.parentRegionSize =
                        m_inputParentSize;
                    region->data.mrp.perms   = m_inputPerms;
                    region->data.mrp.pid     = m_inputPid;
                    region->data.namedValues = m_tempNamedValues;
                }
            }
        }
        else
        {
            if (ImGui::Button("Create Region", ImVec2(-FLT_MIN, 0)))
            {
                MemoryRegionData newData;
                newData.name                      = m_bufName;
                newData.comment                   = m_bufComment;
                newData.mrp.relativeRegionAddress = m_inputAddr;
                newData.mrp.relativeRegionSize    = m_inputSize;
                newData.mrp.parentRegionAddress   = m_inputParentAddr;
                newData.mrp.parentRegionSize      = m_inputParentSize;
                newData.mrp.perms                 = m_inputPerms;
                newData.mrp.pid                   = m_inputPid;
                newData.namedValues               = m_tempNamedValues;
                sp_mg->RegionAdd(newData);
                ResetRegionBuilderState();
            }
        }
    }

    void MemoryGraphViewer::drawLinkEditorTab()
    {
        if (m_isEditingLink)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                               "Editing Link ID: %lu",
                               m_editingLinkID);
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                ResetLinkBuilderState();
                ImNodes::ClearLinkSelection();
            }
        }
        else
        {
            ImGui::Text("Create New Link");
        }

        ImGui::Separator();

        ImGui::InputText("Name", m_linkNameBuf,
                         IM_ARRAYSIZE(m_linkNameBuf));
        ImGui::InputScalar("Source Addr", ImGuiDataType_U64,
                           &m_linkSourceAddr, NULL, NULL, "%016llX",
                           ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::InputScalar("Target Addr", ImGuiDataType_U64,
                           &m_linkTargetAddr, NULL, NULL, "%016llX",
                           ImGuiInputTextFlags_CharsHexadecimal);

        auto policy_names =
            magic_enum::enum_names<rmf::graph::MemoryLinkPolicy>();
        int current_policy_item = static_cast<int>(m_linkPolicy);
        if (ImGui::BeginCombo(
                "Policy",
                std::string(policy_names[current_policy_item])
                    .c_str()))
        {
            for (size_t n = 0; n < policy_names.size(); n++)
            {
                bool is_selected =
                    (current_policy_item == static_cast<int>(n));
                if (ImGui::Selectable(
                        std::string(policy_names[n]).c_str(),
                        is_selected))
                {
                    m_linkPolicy =
                        static_cast<rmf::graph::MemoryLinkPolicy>(n);
                }
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (m_isEditingLink)
        {
            if (ImGui::Button("Save Link", ImVec2(-FLT_MIN, 0)))
            {
                auto optLink = sp_mg->LinkGetFromID(m_editingLinkID);
                if (optLink.has_value())
                {
                    auto* link            = optLink.value();
                    link->data.name       = m_linkNameBuf;
                    link->data.sourceAddr = m_linkSourceAddr;
                    link->data.targetAddr = m_linkTargetAddr;
                    link->data.policy     = m_linkPolicy;
                }
            }
        }
        else
        {
            if (ImGui::Button("Create Link", ImVec2(-FLT_MIN, 0)))
            {
                MemoryRegionLinkData newLink;
                newLink.name       = m_linkNameBuf;
                newLink.sourceAddr = m_linkSourceAddr;
                newLink.targetAddr = m_linkTargetAddr;
                newLink.policy     = m_linkPolicy;
                sp_mg->LinkSmartAdd(newLink);
                ResetLinkBuilderState();
            }
        }

        ImGui::Separator();
        ImGui::Text("Existing Links");
        ImGui::BeginChild("LinkList", ImVec2(0, 0), true);
        for (const auto& link : sp_mg->LinksGetViews())
        {
            ImGui::PushID(link.data.id);
            if (ImGui::Selectable(link.data.name.c_str(),
                                  m_editingLinkID == link.data.id))
            {
                LoadLinkIntoState(link.data);
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
    }

    void MemoryGraphViewer::drawSidebar()
    {
        ImGui::BeginChild("SidebarChild",
                          ImVec2(0, ImGui::GetContentRegionAvail().y),
                          false);
        if (ImGui::BeginTabBar("SidebarTabs"))
        {
            if (ImGui::BeginTabItem("Nodes"))
            {
                drawNodeEditorTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Links"))
            {
                drawLinkEditorTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();
    }

    void
    MemoryGraphViewer::drawNode(const rmf::graph::MemoryRegion& node)
    {
        ImNodes::BeginNode(node.data.id);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(node.data.name.c_str());
        ImNodes::EndNodeTitleBar();

        ImGui::Text("Addr: 0x%lX", node.data.mrp.TrueAddress());
        ImGui::Text("Size: 0x%lX", node.data.mrp.relativeRegionSize);
        ImGui::Text("Perms: %s",
                    std::string(magic_enum::enum_flags_name(
                                    node.data.mrp.perms))
                        .c_str());

        if (!node.data.namedValues.empty())
        {
            // ImGui::Separator();
            for (const auto& val : node.data.namedValues)
            {
                ImGui::Text(
                    "%s: %s",
                    std::string(magic_enum::enum_name(val.type.type))
                        .c_str(),
                    val.name.c_str());
            }
        }
        ImNodes::EndNode();
    }

    void MemoryGraphViewer::drawEditor()
    {
        ImNodes::BeginNodeEditor();
        for (const auto& node : sp_mg->RegionsGetViews())
        {
            drawNode(node);
        }
        // links should be drawn after nodes
        for ([[maybe_unused]] const auto& link :
             sp_mg->LinksGetViews())
        {
            // TODO: This is not correct, we need to find which node contains the source and target address.
            // For now, we will just draw a link between node 0 and 1 if they exist.
            // ImNodes::Link(link.data.id, 0, 1);
        }
        ImNodes::EndNodeEditor();
        if (ImNodes::IsEditorHovered() &&
            ImGui::GetIO().MouseWheel != 0)
        {
            float zoom = ImNodes::EditorContextGetZoom() +
                ImGui::GetIO().MouseWheel * 0.1f;
            ImNodes::EditorContextSetZoom(zoom, ImGui::GetMousePos());
        }
    }

    void MemoryGraphViewer::handlePostFrameLogic(bool deletePressed)
    {
        // Handle Nodes
        int numSelectedNodes = ImNodes::NumSelectedNodes();
        if (numSelectedNodes > 0)
        {
            std::vector<int> selectedNodes(numSelectedNodes);
            ImNodes::GetSelectedNodes(selectedNodes.data());
            int selectedID = selectedNodes[0];

            if (selectedID != static_cast<int>(m_editingID))
            {
                auto optRegion = sp_mg->RegionGetFromID(selectedID);
                if (optRegion.has_value())
                {
                    LoadRegionIntoState(optRegion.value()->data);
                }
            }

            if (deletePressed)
            {
                for (int idToDelete : selectedNodes)
                {
                    sp_mg->RegionDelete(idToDelete);
                    if (idToDelete == static_cast<int>(m_editingID))
                    {
                        ResetRegionBuilderState();
                    }
                }
                ImNodes::ClearNodeSelection();
            }
        }
        else if (m_isEditingMode)
        {
            ResetRegionBuilderState();
        }

        // Handle Links
        int numSelectedLinks = ImNodes::NumSelectedLinks();
        if (numSelectedLinks > 0)
        {
            std::vector<int> selectedLinks(numSelectedLinks);
            ImNodes::GetSelectedLinks(selectedLinks.data());
            int selectedID = selectedLinks[0];

            if (selectedID != static_cast<int>(m_editingLinkID))
            {
                auto optLink = sp_mg->LinkGetFromID(selectedID);
                if (optLink.has_value())
                {
                    LoadLinkIntoState(optLink.value()->data);
                }
            }

            if (deletePressed)
            {
                for (int idToDelete : selectedLinks)
                {
                    sp_mg->LinkDelete(idToDelete);
                    if (idToDelete ==
                        static_cast<int>(m_editingLinkID))
                    {
                        ResetLinkBuilderState();
                    }
                }
                ImNodes::ClearLinkSelection();
            }
        }
        else if (m_isEditingLink)
        {
            ResetLinkBuilderState();
        }
    }

    void MemoryGraphViewer::draw()
    {
        const bool deletePressed =
            ImGui::IsKeyReleased(ImGuiKey_Delete);

        if (ImGui::BeginTable("ViewerSplit", 2,
                              ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn(
                "Sidebar", ImGuiTableColumnFlags_WidthFixed, 300.0f);
            ImGui::TableSetupColumn("Editor",
                                    ImGuiTableColumnFlags_None);

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            drawSidebar();

            ImGui::TableSetColumnIndex(1);
            drawEditor();

            ImGui::EndTable();
        }

        handlePostFrameLogic(deletePressed);
    }
}
