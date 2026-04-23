#include <gtest/gtest.h>
#include <print>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>
#include <rmf/utils/expect.hpp>
#include <rmf/utils/str.hpp>
#include <rmf/region.hpp>
#include <rmf/map.hpp>
#include <rmf/snapshot.hpp>
#include "helpers.hpp"
#include "rmf/utils/function.hpp"

using namespace std;
namespace mf  = RealtimeMemoryForensics;
namespace mfl = mf::Logging;
namespace mfu = mf::Utils;

template <typename T>
concept requiresMap = requires(T t) { t.wellFormed(); };

TEST(snapshot, static_assertions)
{
    mf::Region<mf::Snapshot> gah;
    static_assert(requiresMap<mf::Region<mf::Snapshot>>,
                  "Should require an mf::map!");
    static_assert(
        requires { mf ::Region<mf ::Map, mf ::Snapshot>{}; },
        "Should require an mf::map!");
}
