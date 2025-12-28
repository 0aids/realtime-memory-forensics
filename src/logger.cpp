#include "logger.hpp"
#include <mutex>
#include <pthread.h>
#include <sstream>
#include <thread>
#include <iomanip>

rmf::_LogWrapper::_LogWrapper(const std::string_view &s) {
    m_ss << s;
}

rmf::_LogWrapper::~_LogWrapper() {
    std::lock_guard<std::mutex> lock(logMutex);
    std::cerr << m_ss.str() << rmf::StringColors.at(rmf_Reset) << "\n";
}

rmf::_LogWrapper rmf::Log(rmf_LogLevel level, 
            const std::string_view file,
            const std::string_view func,
            const size_t line) 
{
    // Empty ones should be immediately optimized out.
    if (rmf::g_logLevel < level) return {};

    auto wrapper = std::stringstream();
    auto threadID = std::this_thread::get_id();

    if (threadID != rmf::mainThreadId) {
        char name[20]{};

        pthread_getname_np(pthread_self(), name, sizeof(name));
        wrapper << "[T" << name << "]";
    } else {
        wrapper << "[TM]";
    }
    wrapper << LogLevelNames[level]
        << "[" << file << ":" << line 
        << " - " << func << "] ";
    return _LogWrapper(wrapper.str());
}
