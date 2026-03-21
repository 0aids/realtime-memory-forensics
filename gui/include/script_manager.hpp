#ifndef script_manager_hpp_INCLUDED
#define script_manager_hpp_INCLUDED
#include <thread>
#include <pybind11/embed.h>

namespace rmf::gui
{
    namespace py = pybind11;
    class ScriptManager
    {
      private:
        std::jthread m_pythonWorker;
        std::string  m_scriptBuffer;

      public:
    };
}
#endif // script_manager_hpp_INCLUDED
