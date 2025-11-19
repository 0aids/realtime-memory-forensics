#include "backends/mt_backend.hpp"
#include "utils/logs.hpp"
#include "data/maps.hpp"
#include "backends/core.hpp"
#include "backends/ibackend.hpp"
#include "tests.hpp"
#include "data/snapshots.hpp"
#include <chrono>
#include <thread>
template <typename T>
inline std::span<const T> asSpan(const std::vector<T>& toSpan)
{
    return std::span(toSpan);
}
int main()
{
    using namespace std;
    using namespace std::chrono;
    using namespace rmf::data;
    using namespace rmf::backends;
    using namespace rmf::tests;

    auto pid = runSampleProcess();
    this_thread::sleep_for(500ms);
    auto maps = readMapsFromPid(pid).filterRegionsByPerms("rwp");
    rmf_assert(maps.size() > 0,
               "There should be regions inside the map");
    auto filteredMaps =
        breakIntoRegionChunks(maps.filterRegionsByHasPerms("rwp"), 0);
    MTBackend backend(5);
    {
        auto snaps1 = backend.makeSnapshots(asSpan(filteredMaps));
        rmf_assert(snaps1.size() > 0,
                   "There should be snapshots taken");
        this_thread::sleep_for(50ms);
        auto snaps2 = backend.makeSnapshots(filteredMaps);
        rmf_assert(snaps2.size() > 0,
                   "There should be snapshots taken");
        auto spans1                = mt::makeSnapshotSpans(snaps1);
        auto spans2                = mt::makeSnapshotSpans(snaps2);
        auto changedRegionsResults = backend.findChangingRegions(
            asSpan(spans1), asSpan(spans2), 8);
        rmf_assert(changedRegionsResults.size() > 0,
                   "There is a changing region in there");
        auto strResults = backend.findString(
            asSpan(spans1), asSpan(spans2), "Lorem ipsum");
        rmf_assert(strResults.size() > 0,
                   "There is a Lorem ipsum in there");
        NumQuery query;
        query.min.dle = 0.5;
        query.type = NumType::DLE;
        auto changeNumResults = backend.findChangedNumeric(
            asSpan(spans1), asSpan(spans2), query
        );
        rmf_assert(changeNumResults.size() > 0,
                   "There is a changing float in there.");
    }
}
