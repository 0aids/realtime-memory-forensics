#include "process_manager.hpp"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include "utils.hpp"

namespace rmf::gui
{

    ProcessManager::ProcessManager() : selectedRegionIndex(-1)
    {
        refreshProcessList();
    }

    void ProcessManager::refreshProcessList()
    {
        processList.clear();
        const std::string procPath = "/proc";

        for (const auto& entry :
             std::filesystem::directory_iterator(procPath))
        {
            std::string name = entry.path().filename();
            if (std::all_of(name.begin(), name.end(), ::isdigit))
            {
                pid_t       pid = static_cast<pid_t>(std::stoi(name));
                std::string commPath =
                    entry.path().string() + "/comm";
                std::string   procName = name;

                std::ifstream commFile(commPath);
                if (commFile.is_open())
                {
                    std::getline(commFile, procName);
                    commFile.close();
                }

                processList.emplace_back(pid, procName);
            }
        }

        std::sort(processList.begin(), processList.end(),
                  [](const auto& a, const auto& b)
                  { return a.second < b.second; });
    }

    void ProcessManager::attachToProcess(pid_t pid)
    {
        std::string commPath =
            "/proc/" + std::to_string(pid) + "/comm";
        std::ifstream commFile(commPath);
        if (commFile.is_open())
        {
            std::getline(commFile, targetProcessName);
            commFile.close();
        }
        else
        {
            targetProcessName = "Unknown";
        }

        targetPid = pid;

        std::string mapsPath =
            "/proc/" + std::to_string(pid) + "/maps";
        currentRegions      = rmf::utils::ParseMaps(mapsPath);
        selectedRegionIndex = -1;
    }

    void ProcessManager::detachFromProcess()
    {
        targetPid.reset();
        targetProcessName.clear();
        currentRegions.clear();
        selectedRegionIndex = -1;
    }

    void ProcessManager::draw()
    {
        if (ImGui::Begin("Process Attachment"))
        {
            if (targetPid.has_value())
            {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
                                   "PID: %d", targetPid.value());
                ImGui::SameLine();
                ImGui::Text("Name: %s", targetProcessName.c_str());
                if (ImGui::Button("Detach"))
                {
                    detachFromProcess();
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                                   "No process attached");

                static int manualPid = 0;
                ImGui::InputInt("PID", &manualPid);
                ImGui::SameLine();
                if (ImGui::Button("Attach"))
                {
                    if (manualPid > 0)
                    {
                        attachToProcess(manualPid);
                        manualPid = 0;
                    }
                }

                ImGui::Separator();
                ImGui::Text("Process Browser:");
                if (ImGui::Button("Refresh"))
                {
                    refreshProcessList();
                }

                static int selectedProcess = -1;
                float      listHeight =
                    ImGui::GetTextLineHeightWithSpacing() * 10;
                if (ImGui::BeginListBox("Processes",
                                        ImVec2(-FLT_MIN, listHeight)))
                {
                    for (int i = 0;
                         i < static_cast<int>(processList.size());
                         i++)
                    {
                        std::string label =
                            std::to_string(processList[i].first) +
                            " - " + processList[i].second;
                        if (ImGui::Selectable(
                                label.c_str(), selectedProcess == i,
                                ImGuiSelectableFlags_AllowDoubleClick))
                        {
                            selectedProcess = i;
                            if (ImGui::IsMouseDoubleClicked(
                                    ImGuiMouseButton_Left))
                            {
                                attachToProcess(processList[i].first);
                                selectedProcess = -1;
                            }
                        }
                    }
                    ImGui::EndListBox();
                }

                if (selectedProcess >= 0 &&
                    selectedProcess <
                        static_cast<int>(processList.size()))
                {
                    ImGui::SameLine();
                    if (ImGui::Button("Attach##selected"))
                    {
                        attachToProcess(
                            processList[selectedProcess].first);
                        selectedProcess = -1;
                    }
                }
            }
        }
        ImGui::End();
    }

}
