#ifndef PROCESS_MANAGER_HPP
#define PROCESS_MANAGER_HPP

#include <vector>
#include <string>
#include <utility>
#include <optional>
#include <unistd.h>
#include <imgui.h>
#include "types.hpp"

namespace rmf::gui
{

    class ProcessManager
    {
      public:
        ProcessManager();
        ~ProcessManager() = default;

        // Process browser
        std::vector<std::pair<pid_t, std::string>> processList;
        void refreshProcessList();
        void attachToProcess(pid_t pid);
        void detachFromProcess();

        // Getters
        std::optional<pid_t> getTargetPid() const
        {
            return targetPid;
        }
        const std::string& getTargetProcessName() const
        {
            return targetProcessName;
        }
        const types::MemoryRegionPropertiesVec&
        getCurrentRegions() const
        {
            return currentRegions;
        }

        // UI
        void draw();

      private:
        std::optional<pid_t>             targetPid;
        std::string                      targetProcessName;
        types::MemoryRegionPropertiesVec currentRegions;
        int                              selectedRegionIndex;
    };

}

#endif // PROCESS_MANAGER_HPP
