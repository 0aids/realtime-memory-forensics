#include "memory_graph_viewer.hpp"
#include "imgui.h"
#include "imnodes.h"
#include "memory_graph.hpp"
#include <magic_enum/magic_enum.hpp>

namespace rmf::graph
{
    // Helper to generate a unique integer ID for a node's input attribute
    int GetNodeInputAttributeId(MemoryRegionID nodeId)
    {
        return (static_cast<int>(nodeId) << 16) | 1;
    }

    // Helper to generate a unique integer ID for a node's output attribute based on offset
    int GetNodeOutputAttributeId(MemoryRegionID nodeId,
                                 ptrdiff_t      offset = 0)
    {
        return (static_cast<int>(nodeId) << 16) |
            (static_cast<int>(offset) & 0xFFF) | 2;
    }

    void MemoryGraphViewer::ResetRegionBuilderState()
    {
        m_isEditingMode = false;
        m_editingID     = -1;

        snprintf(m_bufName, sizeof(m_bufName), "New Region");
        m_bufComment[0]   = '\0';
        m_inputAddr       = 0x0;
        m_inputSize       = 1024;
        m_inputParentAddr = 0;
        m_inputParentSize = 1024;
        m_inputPerms      = rmf::types::Perms::None;
        m_inputPid        = 0;
        m_tempNamedValues.clear();
    }

    void MemoryGraphViewer::ResetLinkBuilderState()
    {
        m_isEditingLink = false;
        m_editingLinkID = noID_ce;
        snprintf(m_linkNameBuf, sizeof(m_linkNameBuf), "New Link");
        m_linkSourceAddr = 0x0;
        m_linkTargetAddr = 0x0;
        m_linkPolicy     = rmf::graph::MemoryLinkPolicy::Strict;
    }

    void MemoryGraphViewer::LoadRegionIntoState(
        const rmf::graph::MemoryRegion& region)
    {
        m_isEditingMode = true;
        m_editingID     = region.id;
        m_isEditingLink = false; // Unselect any selected link
        m_editingLinkID = noID_ce;

        strncpy(m_bufName, region.data.name.c_str(),
                sizeof(m_bufName) - 1);
        m_bufName[sizeof(m_bufName) - 1] = '\0';
        strncpy(m_bufComment, region.data.comment.c_str(),
                sizeof(m_bufComment) - 1);
        m_bufComment[sizeof(m_bufComment) - 1] = '\0';

        m_inputAddr       = region.data.mrp.relativeRegionAddress;
        m_inputSize       = region.data.mrp.relativeRegionSize;
        m_inputParentAddr = region.data.mrp.parentRegionAddress;
        m_inputParentSize = region.data.mrp.parentRegionSize;
        m_inputPerms      = region.data.mrp.perms;
        m_tempNamedValues = region.data.namedValues;
    }

    void MemoryGraphViewer::LoadLinkIntoState(
        const rmf::graph::MemoryLink& link)
    {
        m_isEditingLink = true;
        m_editingLinkID = link.id;
        m_isEditingMode = false; // Unselect any selected node
        m_editingID     = -1;

        strncpy(m_linkNameBuf, link.data.name.c_str(),
                sizeof(m_linkNameBuf) - 1);
        m_linkNameBuf[sizeof(m_linkNameBuf) - 1] = '\0';

        m_linkSourceAddr = link.data.sourceAddr;
        m_linkTargetAddr = link.data.targetAddr;
        m_linkPolicy     = link.data.policy;
    }

    void MemoryGraphViewer::drawNodeEditorTab()
    {
        using namespace magic_enum::bitwise_operators;
        if (m_isEditingMode)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                               "Editing Region ID: %lu", m_editingID);
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                ResetRegionBuilderState();
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
        if (ImGui::Button("+ Add Value"))
        {
            m_tempNamedValues.push_back(
                {"Var", rmf::types::type{rmf::types::typeName::_u64},
                 0x0, ""});
        }
        if (ImGui::BeginTable("VarsList", 4,
                              ImGuiTableFlags_Borders |
                                  ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Offset");
            ImGui::TableSetupColumn(
                "Del", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableHeadersRow();
            for (size_t i = 0; i < m_tempNamedValues.size();)
            {
                ImGui::PushID(i);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                char varNameBuf[64];
                strncpy(varNameBuf, m_tempNamedValues[i].name.c_str(),
                        sizeof(varNameBuf));
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##vn", varNameBuf,
                                     sizeof(varNameBuf)))
                {
                    m_tempNamedValues[i].name = varNameBuf;
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);
                auto type_names =
                    magic_enum::enum_names<rmf::types::typeName>();
                int current_item =
                    static_cast<int>(m_tempNamedValues[i].type.type);
                if (ImGui::BeginCombo(
                        "##type",
                        std::string(type_names[current_item])
                            .c_str()))
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

                ImGui::TableSetColumnIndex(2);
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::InputScalar(
                    "##vo", ImGuiDataType_S64,
                    &m_tempNamedValues[i].offset, NULL, NULL, "%X",
                    ImGuiInputTextFlags_CharsHexadecimal);

                ImGui::TableSetColumnIndex(3);
                if (ImGui::Button("x"))
                {
                    m_tempNamedValues.erase(
                        m_tempNamedValues.begin() + i);
                    ImGui::PopID();
                    continue;
                }
                ImGui::PopID();
                i++;
            }
            ImGui::EndTable();
        }

        ImGui::Dummy(ImVec2(0, 10));

        if (!m_nodeEditorError.empty())
        {
            ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f),
                               "Error: %s",
                               m_nodeEditorError.c_str());
        }

        if (m_isEditingMode)
        {
            if (ImGui::Button("Save Changes", ImVec2(-FLT_MIN, 0)))
            {
                if (m_inputAddr >= m_inputParentSize ||
                    m_inputSize > (m_inputParentSize - m_inputAddr))
                {
                    m_nodeEditorError =
                        "Relative region exceeds parent boundaries!";
                }
                else
                {
                    m_nodeEditorError.clear();
                    auto optRegion =
                        sp_mg->RegionGetFromID(m_editingID);
                    if (optRegion.has_value())
                    {
                        MemoryRegion* region = optRegion.value();
                        region->data.name    = m_bufName;
                        region->data.comment = m_bufComment;
                        region->data.mrp.relativeRegionAddress =
                            m_inputAddr;
                        region->data.mrp.relativeRegionSize =
                            m_inputSize;
                        region->data.mrp.parentRegionAddress =
                            m_inputParentAddr;
                        region->data.mrp.parentRegionSize =
                            m_inputParentSize;
                        region->data.mrp.perms   = m_inputPerms;
                        region->data.namedValues = m_tempNamedValues;
                    }
                }
            }
        }
        else
        {
            if (ImGui::Button("Create Region", ImVec2(-FLT_MIN, 0)))
            {
                if (m_inputAddr >= m_inputParentSize ||
                    m_inputSize > (m_inputParentSize - m_inputAddr))
                {
                    m_nodeEditorError =
                        "Relative region exceeds parent boundaries!";
                }
                else
                {
                    m_nodeEditorError.clear();
                    MemoryRegionData newData;
                    newData.name                      = m_bufName;
                    newData.comment                   = m_bufComment;
                    newData.mrp.relativeRegionAddress = m_inputAddr;
                    newData.mrp.relativeRegionSize    = m_inputSize;
                    newData.mrp.parentRegionAddress =
                        m_inputParentAddr;
                    newData.mrp.parentRegionSize = m_inputParentSize;
                    newData.mrp.perms            = m_inputPerms;
                    newData.namedValues          = m_tempNamedValues;
                    sp_mg->RegionAdd(newData);
                    ResetRegionBuilderState();
                }
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
                    sp_mg->LinkSmartAdd(link->data);
                }
            }
        }
        else
        {
            if (ImGui::Button("Create Link", ImVec2(-FLT_MIN, 0)))
            {
                MemoryLinkData newLink;
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
        if (ImGui::BeginTable("LinkList", 3,
                              ImGuiTableFlags_Borders |
                                  ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Source");
            ImGui::TableSetupColumn("Target");
            ImGui::TableHeadersRow();

            for (const auto& link : sp_mg->LinksGetViews())
            {
                ImGui::PushID(link.id);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                if (ImGui::Selectable(
                        link.data.name.c_str(),
                        m_editingLinkID == link.id,
                        ImGuiSelectableFlags_SpanAllColumns))
                {
                    LoadLinkIntoState(link);
                }

                ImGui::TableSetColumnIndex(1);
                auto sourceNode =
                    sp_mg->RegionGetFromID(link.data.sourceID);
                if (sourceNode.has_value())
                {
                    ImGui::TextUnformatted(
                        sourceNode.value()->data.name.c_str());
                }
                else
                {
                    ImGui::Text("N/A");
                }

                ImGui::TableSetColumnIndex(2);
                auto targetNode =
                    sp_mg->RegionGetFromID(link.data.targetID);
                if (targetNode.has_value())
                {
                    ImGui::TextUnformatted(
                        targetNode.value()->data.name.c_str());
                }
                else
                {
                    ImGui::Text("N/A");
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
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
                m_editorState = State::NodeEditor;
                drawNodeEditorTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Links"))
            {
                m_editorState = State::LinkEditor;
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
        ImNodes::BeginNode(node.id);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(node.data.name.c_str());
        ImNodes::EndNodeTitleBar();

        ImNodes::BeginInputAttribute(
            GetNodeInputAttributeId(node.id));
        ImGui::Text("Default In");
        ImNodes::EndInputAttribute();

        ImNodes::BeginOutputAttribute(
            GetNodeOutputAttributeId(node.id));
        ImGui::Text("Default Out");
        ImNodes::EndOutputAttribute();

        ImGui::Text("Addr: 0x%lX", node.data.mrp.TrueAddress());
        ImGui::Text("Size: 0x%lX", node.data.mrp.relativeRegionSize);
        ImGui::Text("Perms: %s",
                    std::string(magic_enum::enum_flags_name(
                                    node.data.mrp.perms))
                        .c_str());

        if (!node.data.namedValues.empty())
        {
            ImGui::Separator();
            for (const auto& val : node.data.namedValues)
            {
                if (val.type.type == rmf::types::typeName::_ptr)
                {
                    // ImNodes::BeginOutputAttribute(
                    //     GetNodeOutputAttributeId(node.id,
                    //                              val.offset));
                    ImGui::Text("%s: %s",
                                std::string(magic_enum::enum_name(
                                                val.type.type))
                                    .c_str(),
                                val.name.c_str());
                    // ImNodes::EndOutputAttribute();
                }
                else
                {
                    ImGui::Text("%s: %s",
                                std::string(magic_enum::enum_name(
                                                val.type.type))
                                    .c_str(),
                                val.name.c_str());
                }
            }
        }
        ImNodes::EndNode();
    }

    void
    MemoryGraphViewer::drawLink(const rmf::graph::MemoryLink& link)
    {
        if (link.data.sourceID != noID_ce &&
            link.data.targetID != noID_ce)
        {
            auto sourceNodeOpt =
                sp_mg->RegionGetFromID(link.data.sourceID);
            if (!sourceNodeOpt.has_value())
                return;

            ptrdiff_t offset = link.data.sourceAddr -
                sourceNodeOpt.value()->data.mrp.TrueAddress();

            ImNodes::Link(
                link.id,
                GetNodeOutputAttributeId(link.data.sourceID),
                GetNodeInputAttributeId(link.data.targetID));
        }
    }

    void MemoryGraphViewer::drawEditor()
    {
        ImNodes::BeginNodeEditor();
        for (const auto& node : sp_mg->RegionsGetViews())
        {
            drawNode(node);
        }

        for (const auto& link : sp_mg->LinksGetViews())
        {
            drawLink(link);
        }
        ImNodes::MiniMap(0.2, ImNodesMiniMapLocation_BottomRight);
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
        int numSelectedNodes = ImNodes::NumSelectedNodes();
        if (numSelectedNodes > 0 &&
            m_editorState == State::NodeEditor)
        {
            ResetLinkBuilderState(); // Unselect link if node is selected from canvas
            std::vector<int> selectedNodes(numSelectedNodes);
            ImNodes::GetSelectedNodes(selectedNodes.data());
            int selectedID = selectedNodes[0];

            if (selectedID != static_cast<int>(m_editingID))
            {
                auto optRegion = sp_mg->RegionGetFromID(selectedID);
                if (optRegion.has_value())
                {
                    LoadRegionIntoState(*optRegion.value());
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

        int numSelectedLinks = ImNodes::NumSelectedLinks();
        if (numSelectedLinks > 0 &&
            m_editorState == State::LinkEditor)
        {
            ResetRegionBuilderState(); // Unselect node if link is selected from canvas
            std::vector<int> selectedLinks(numSelectedLinks);
            ImNodes::GetSelectedLinks(selectedLinks.data());
            int selectedID = selectedLinks[0];

            if (selectedID != static_cast<int>(m_editingLinkID))
            {
                auto optLink = sp_mg->LinkGetFromID(selectedID);
                if (optLink.has_value())
                {
                    LoadLinkIntoState(*optLink.value());
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

        // Deselect if clicking on empty canvas background
        int  hovered_node_id = -1;
        int  hovered_link_id = -1;
        bool is_node_hovered =
            ImNodes::IsNodeHovered(&hovered_node_id);
        bool is_link_hovered =
            ImNodes::IsLinkHovered(&hovered_link_id);
        if (ImNodes::IsEditorHovered() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !is_node_hovered && !is_link_hovered)
        {
            ResetRegionBuilderState();
            ResetLinkBuilderState();
        }
    }

    void MemoryGraphViewer::draw()
    {
        bool deletePressed = ImGui::IsKeyReleased(ImGuiKey_Delete);
        // Check if we are editing an link, if so, delete it.
        if (deletePressed && m_editingLinkID != noID_ce)
        {
            sp_mg->LinkDelete(m_editingLinkID);
            deletePressed = false;
            ResetLinkBuilderState();
        }

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
