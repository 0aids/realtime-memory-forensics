#include "rmf.hpp"

namespace rmf {
    Analyzer::Analyzer(size_t numThreads) {
        // I swear to god I keep forgetting these fucking template arguments.
        m_impl = std::make_shared<Impl>(numThreads);
    }
}
