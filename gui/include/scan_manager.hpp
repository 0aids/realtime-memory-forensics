#ifndef SCAN_MANAGER_HPP
#define SCAN_MANAGER_HPP

#include <vector>
#include <string>
#include <chrono>
#include <imgui.h>
#include "gui_types.hpp" // For ScanOperation, ScanValueType, ScanResult
#include "rmf.hpp"       // For Analyzer

namespace rmf::gui
{

    class ProcessManager; // Forward declaration

    class ScanManager
    {
      public:
        ScanManager(ProcessManager& processMgr);
        ~ScanManager() = default;

        // Operations
        void runScan();
        void clearScanResults();

        // Getters
        const std::vector<ScanResult>& getScanResults() const
        {
            return scanResults;
        }
        bool isScanning() const
        {
            return m_isScanning;
        }

        // UI
        void draw(std::function<void()> callback = {});

      private:
        ProcessManager&                processManager;
        std::unique_ptr<rmf::Analyzer> analyzer;
        size_t                         analyzerThreadCount;

        // Scan state
        bool          showScanWindow   = true;
        ScanOperation scanOperation    = ScanOperation::ExactValue;
        ScanValueType scanValueType    = ScanValueType::i32;
        char          scanValueBuf[64] = "";
        bool          scanHexInput     = false;
        bool          m_isScanning     = false;
        std::vector<ScanResult> scanResults;
        int                     selectedResultIndex = -1;
        std::chrono::steady_clock::time_point scanStartTime;
        std::chrono::milliseconds             scanDuration{0};
    };

}

#endif // SCAN_MANAGER_HPP
