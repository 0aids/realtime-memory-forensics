#include "scan_manager.hpp"
#include "process_manager.hpp"
#include <magic_enum/magic_enum.hpp>
#include "operations.hpp"

namespace rmf::gui
{

    ScanManager::ScanManager(ProcessManager& processMgr) :
        processManager(processMgr), analyzerThreadCount(6)
    {
        analyzer =
            std::make_unique<rmf::Analyzer>(analyzerThreadCount);
    }

    void ScanManager::clearScanResults()
    {
        scanResults.clear();
        selectedResultIndex = -1;
    }

    void ScanManager::runScan()
    {
        if (!processManager.getTargetPid().has_value() ||
            processManager.getCurrentRegions().empty())
            return;

        clearScanResults();
        m_isScanning  = true;
        scanStartTime = std::chrono::steady_clock::now();

        pid_t pid = processManager.getTargetPid().value();

        // Filter readable regions for scanning
        auto readableRegions =
            processManager.getCurrentRegions().FilterHasPerms("r");

        // Take snapshots - wrap in lambda to pass pid
        auto makeSnap = [&](const types::MemoryRegionProperties& mrp)
        { return types::MemorySnapshot::Make(mrp, pid); };

        auto snaps = analyzer->Execute(makeSnap, readableRegions);

        std::vector<types::MemoryRegionPropertiesVec> results;

        // Parse the input value
        std::string valueStr(scanValueBuf);

        try
        {
            switch (scanOperation)
            {
                case ScanOperation::ExactValue:
                {
                    if (scanValueType == ScanValueType::i32)
                    {
                        int32_t val = static_cast<int32_t>(
                            std::stoll(valueStr, nullptr,
                                       scanHexInput ? 16 : 10));
                        auto scanOp =
                            [&](const types::MemorySnapshot& snap)
                        {
                            return rmf::op::findNumeralExact<int32_t>(
                                snap, val);
                        };
                        results = analyzer->Execute(scanOp, snaps);
                    }
                    else if (scanValueType == ScanValueType::i64)
                    {
                        int64_t val =
                            std::stoll(valueStr, nullptr,
                                       scanHexInput ? 16 : 10);
                        auto scanOp =
                            [&](const types::MemorySnapshot& snap)
                        {
                            return rmf::op::findNumeralExact<int64_t>(
                                snap, val);
                        };
                        results = analyzer->Execute(scanOp, snaps);
                    }
                    else if (scanValueType == ScanValueType::u32)
                    {
                        uint32_t val = static_cast<uint32_t>(
                            std::stoull(valueStr, nullptr,
                                        scanHexInput ? 16 : 10));
                        auto scanOp =
                            [&](const types::MemorySnapshot& snap)
                        {
                            return rmf::op::findNumeralExact<
                                uint32_t>(snap, val);
                        };
                        results = analyzer->Execute(scanOp, snaps);
                    }
                    else if (scanValueType == ScanValueType::u64)
                    {
                        uint64_t val =
                            std::stoull(valueStr, nullptr,
                                        scanHexInput ? 16 : 10);
                        auto scanOp =
                            [&](const types::MemorySnapshot& snap)
                        {
                            return rmf::op::findNumeralExact<
                                uint64_t>(snap, val);
                        };
                        results = analyzer->Execute(scanOp, snaps);
                    }
                    break;
                }
                case ScanOperation::String:
                {
                    std::string_view sv = valueStr;
                    auto             scanOp =
                        [&](const types::MemorySnapshot& snap)
                    { return rmf::op::findString(snap, sv); };
                    results = analyzer->Execute(scanOp, snaps);
                    break;
                }
                default: break;
            }
        }
        catch (const std::exception& e)
        {
            rmf_Log(rmf_Error, "Scan error: " << e.what());
        }

        // Flatten results and convert to ScanResult
        for (const auto& resultVec : results)
        {
            for (const auto& mrp : resultVec)
            {
                ScanResult sr;
                sr.address  = mrp.TrueAddress();
                sr.valueStr = valueStr;
                sr.mrp      = mrp;
                scanResults.push_back(sr);
            }
        }

        scanDuration =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - scanStartTime);
        m_isScanning = false;
    }

    void ScanManager::draw(std::function<void()> callback)
    {
        if (ImGui::Begin("Scan", &showScanWindow))
        {
            // Operation selector
            const char* ops[]     = {"Exact Value", "Changed",
                                     "Unchanged", "String",
                                     "Pointer to Region"};
            int         currentOp = static_cast<int>(scanOperation);
            if (ImGui::Combo("Operation", &currentOp, ops,
                             IM_ARRAYSIZE(ops)))
            {
                scanOperation = static_cast<ScanOperation>(currentOp);
            }

            // Value type selector (only for numeric operations)
            if (scanOperation == ScanOperation::ExactValue ||
                scanOperation == ScanOperation::Changed ||
                scanOperation == ScanOperation::Unchanged)
            {
                const char* types[] = {"i8",  "i16", "i32", "i64",
                                       "u8",  "u16", "u32", "u64",
                                       "f32", "f64"};
                int currentType     = static_cast<int>(scanValueType);
                if (ImGui::Combo("Type", &currentType, types,
                                 IM_ARRAYSIZE(types)))
                {
                    scanValueType =
                        static_cast<ScanValueType>(currentType);
                }
            }

            // Value input
            ImGui::Checkbox("Hex", &scanHexInput);
            ImGui::SameLine();

            const char* hint = scanHexInput ? "0xDEADBEEF" : "12345";
            ImGui::InputText("Value", scanValueBuf,
                             sizeof(scanValueBuf),
                             ImGuiInputTextFlags_CharsHexadecimal);

            // Scan button
            if (!m_isScanning &&
                ImGui::Button("Run Scan", ImVec2(100, 30)))
            {
                runScan();
            }

            if (m_isScanning)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                                   "Scanning...");
            }

            ImGui::Separator();

            // Results
            ImGui::Text("Results: %zu (%.2f ms)", scanResults.size(),
                        std::chrono::duration<double, std::milli>(
                            scanDuration)
                            .count());

            if (!scanResults.empty())
            {
                // TODO: Move "Add All to Graph" to GraphManager interaction
                if (ImGui::Button("Clear"))
                {
                    clearScanResults();
                }
            }

            float tableHeight = ImGui::GetContentRegionAvail().y - 60;
            if (tableHeight < 100)
                tableHeight = 100;

            if (ImGui::BeginTable("ScanResults", 3,
                                  ImGuiTableFlags_Borders |
                                      ImGuiTableFlags_Resizable |
                                      ImGuiTableFlags_RowBg |
                                      ImGuiTableFlags_ScrollY,
                                  ImVec2(-FLT_MIN, tableHeight)))
            {
                ImGui::TableSetupColumn("Address");
                ImGui::TableSetupColumn("Value");
                ImGui::TableSetupColumn("Region");
                ImGui::TableHeadersRow();

                for (int i = 0;
                     i < static_cast<int>(scanResults.size()); i++)
                {
                    const auto& result = scanResults[i];
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    bool isSelected = (selectedResultIndex == i);
                    std::string addrStr =
                        std::format("0x{:x}", result.address);
                    if (ImGui::Selectable(
                            addrStr.c_str(), isSelected,
                            ImGuiSelectableFlags_SpanAllColumns))
                    {
                        selectedResultIndex = i;
                    }
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(result.valueStr.c_str());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted(
                        result.mrp.regionName_sp->c_str());
                }
                ImGui::EndTable();
            }

            if (selectedResultIndex >= 0 &&
                selectedResultIndex <
                    static_cast<int>(scanResults.size()))
            {
                ImGui::Separator();
                const auto& result = scanResults[selectedResultIndex];
                ImGui::Text("Selected: 0x%lx", result.address);
                // TODO: Add to Graph button logic moved to GraphManager
            }
            callback();
        }
        ImGui::End();
    }

}
