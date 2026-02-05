#include "memory_graph_viewer.hpp"
#include "imgui.h"
#include "imnodes.h"
#include "memory_graph.hpp"

namespace rmf::graph
{
    void MemoryGraphViewer::ResetRegionBuilderState()
    {
        m_isEditingMode = false;
        m_editingID     = -1;

        snprintf(m_bufName, sizeof(m_bufName), "New Region");
        m_bufComment[0] = '\0';
        m_inputAddr     = 0x0;
        m_inputSize     = 1024;
        m_inputPid      = 0;
        m_tempNamedValues.clear();
    }

    void MemoryGraphViewer::LoadRegionIntoState(
        const rmf::graph::MemoryRegionData& data)
    {
        m_isEditingMode = true;
        m_editingID     = data.id;

        // Copy strings safely
        strncpy(m_bufName, data.name.c_str(), sizeof(m_bufName) - 1);
        m_bufName[sizeof(m_bufName) - 1] = '\0';

        strncpy(m_bufComment, data.comment.c_str(),
                sizeof(m_bufComment) - 1);
        m_bufComment[sizeof(m_bufComment) - 1] = '\0';

        // Copy scalars
        m_inputAddr = data.mrp.relativeRegionAddress;
        m_inputSize = data.mrp.relativeRegionSize;
        m_inputPid  = data.mrp.pid;

        // Copy vector
        m_tempNamedValues = data.namedValues;
    }

    void MemoryGraphViewer::draw()
    {
        // Check for DEL key before doing anything else
        // Note: We check this before rendering because if we delete the node,
        // we don't want to try and render it in the loop below.
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

            // ==========================================================
            // COLUMN 1: SIDEBAR (Create / Edit Menu)
            // ==========================================================
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            ImGui::BeginChild(
                "RegionBuilderSidebar",
                ImVec2(0, ImGui::GetContentRegionAvail().y), false);

            // Header changes based on mode
            if (m_isEditingMode)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                                   "Editing Region ID: %d",
                                   m_editingID);
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    ResetRegionBuilderState();
                    ImNodes::ClearNodeSelection();
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
            ImGui::InputScalar("Addr", ImGuiDataType_U64, &m_inputAddr,
                               NULL, NULL, "%016llX",
                               ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::InputScalar("Size", ImGuiDataType_U64, &m_inputSize);

            ImGui::Spacing();
            ImGui::Text("Named Values");
            ImGui::Separator();

            // Vars List
            ImGui::BeginChild("VarsList", ImVec2(0, 150), true);
            if (ImGui::Button("+ Add Value"))
            {
                m_tempNamedValues.push_back({"Var", 0x0, ""});
            }
            for (int i = 0; i < m_tempNamedValues.size(); ++i)
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
                ImGui::SetNextItemWidth(60);
                ImGui::InputScalar(
                    "##vo", ImGuiDataType_S64,
                    &m_tempNamedValues[i].offset, NULL, NULL, "%X",
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

            // ACTION BUTTON (Create vs Save)
            if (m_isEditingMode)
            {
                if (ImGui::Button("Save Changes",
                                  ImVec2(-FLT_MIN, 0)))
                {
                    // Retrieve the existing pointer
                    auto optRegion =
                        sp_mg->RegionGetFromID(m_editingID);
                    if (optRegion.has_value())
                    {
                        MemoryRegion* region = optRegion.value();

                        // Update Data
                        region->data.name    = m_bufName;
                        region->data.comment = m_bufComment;
                        region->data.mrp.relativeRegionAddress =
                            m_inputAddr;
                        region->data.mrp.relativeRegionSize =
                            m_inputSize;
                        region->data.mrp.pid     = m_inputPid;
                        region->data.namedValues = m_tempNamedValues;
                    }
                    // We keep edit mode active so user can keep tweaking,
                    // or you could call ResetRegionBuilderState() here to deselect.
                }
            }
            else
            {
                if (ImGui::Button("Create Region",
                                  ImVec2(-FLT_MIN, 0)))
                {
                    MemoryRegionData newData;
                    newData.name                      = m_bufName;
                    newData.comment                   = m_bufComment;
                    newData.parentGraph               = nullptr;
                    newData.mrp.relativeRegionAddress = m_inputAddr;
                    newData.mrp.relativeRegionSize    = m_inputSize;
                    newData.mrp.pid                   = m_inputPid;
                    newData.namedValues = m_tempNamedValues;

                    sp_mg->AddRegion(newData);
                    ResetRegionBuilderState();
                }
            }

            ImGui::EndChild(); // End Sidebar

            // ==========================================================
            // COLUMN 2: NODE EDITOR
            // ==========================================================
            ImGui::TableSetColumnIndex(1);

            ImNodes::BeginNodeEditor();

            // 1. Draw Nodes
            for (const auto& node : sp_mg->RegionsGetViews())
            {
                ImNodes::BeginNode(node.data.id);
                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(node.data.name.c_str());
                ImNodes::EndNodeTitleBar();

                ImGui::Text("0x%lX",
                            node.data.mrp.relativeRegionAddress);
                if (!node.data.namedValues.empty())
                {
                    ImGui::Separator();
                    for (const auto& val : node.data.namedValues)
                    {
                        ImGui::Text("%s", val.name.c_str());
                    }
                }
                ImNodes::EndNode();
            }

            ImNodes::EndNodeEditor();

            // ==========================================================
            // POST-FRAME LOGIC: SELECTION & DELETION
            // ==========================================================

            int numSelected = ImNodes::NumSelectedNodes();

            // A. Handle Selection Changes
            if (numSelected > 0)
            {
                // Get the first selected node
                std::vector<int> selectedNodes(numSelected);
                ImNodes::GetSelectedNodes(selectedNodes.data());
                int selectedID = selectedNodes[0];

                // If the selection is different from what we are currently editing, load it
                if (selectedID != m_editingID)
                {
                    auto optRegion =
                        sp_mg->RegionGetFromID(selectedID);
                    if (optRegion.has_value())
                    {
                        LoadRegionIntoState(optRegion.value()->data);
                    }
                }

                // B. Handle Deletion
                if (deletePressed)
                {
                    // Delete all selected nodes
                    for (int idToDelete : selectedNodes)
                    {
                        sp_mg->RegionDelete(idToDelete);

                        // If we just deleted the node we were editing, reset the sidebar
                        if (idToDelete == m_editingID)
                        {
                            ResetRegionBuilderState();
                        }
                    }
                    ImNodes::ClearNodeSelection();
                }
            }
            else
            {
                // If nothing is selected, check if we *should* clear edit mode.
                // ImNodes keeps selection state until background is clicked.
                // If numSelected == 0, it means the user clicked the background
                // or cleared selection manually.
                if (m_isEditingMode)
                {
                    ResetRegionBuilderState();
                }
            }

            ImGui::EndTable();
        }
    }
}
