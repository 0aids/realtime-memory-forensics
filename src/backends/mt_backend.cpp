#include "backends/mt_backend.hpp"
#include "backends/core.hpp"
#include "utils/logs.hpp"
#include "data/snapshots.hpp"
#include "backends/ibackend.hpp"
#include <type_traits>

namespace rmf::backends::mt {

inline std::span<const char>
spanFromRegionProperties(const MemoryRegionProperties& mrp,
                         const MemorySnapshot&         snap)
{
    return snap.asSpan().subspan(mrp.relativeRegionStart,
                                 mrp.relativeRegionStart +
                                     mrp.relativeRegionSize);
}

RegionPropertiesList
divideSingleRegion(const MemoryRegionProperties& mrp, size_t quantity)
{
    std::vector<MemoryRegionProperties> mrpVec;
    size_t avgSize = mrp.relativeRegionSize / quantity;

    for (size_t i = 0; i < quantity; i++)
    {
        MemoryRegionProperties newMrp = mrp;

        // Ensure the last one is of correct size.
        newMrp.relativeRegionSize = (i != quantity - 1) ?
            avgSize :
            mrp.relativeRegionSize - avgSize * i;
        newMrp.relativeRegionStart =
            mrp.relativeRegionStart + avgSize * i;
        mrpVec.push_back(newMrp);
    }
    return mrpVec;
}

std::vector<MemorySnapshotSpan> divideSingleSnapshot(const MemorySnapshot &snap, size_t quantity) {
    std::vector<MemorySnapshotSpan> spanVec;
    size_t avgSize = snap.size() / quantity;

    for (size_t i = 0; i < quantity; i++)
    {
        size_t numBytes = (i == quantity - 1) ? snap.size() - avgSize * i :
            avgSize;
        size_t offset = 
            avgSize * i;
        spanVec.push_back(snap.asSnapshotSpan().subspan(
            offset,
            numBytes
        ));
        rmf_Log(rmf_Debug, std::hex << std::showbase <<
        "Push back offset: " << offset << "\tcount: " << numBytes);
    }
    return spanVec;
}


std::vector<CoreInputs> consolidateIntoCoreInput(
    const MultipleCoreInputs &c
) {
    std::vector<CoreInputs> coreInputsVec;
    size_t c1 = 0;
    size_t c2 = 0;
    size_t c3 = 0;

    size_t total = 1;
    while (total) {
        total = 0;
        CoreInputs cin;
        if (c.mrpVec && c1 < c.mrpVec.value().size()) {
            cin.mrp.emplace(c.mrpVec.value()[c1]);
            c1++;
            total++;
        }
        if (c.snap1Vec && c2 < c.snap1Vec.value().size()) {
            cin.snap1.emplace(c.snap1Vec.value()[c2]);
            c2++;
            total++;
        }
        if (c.snap2Vec && c3 < c.snap2Vec.value().size()) {
            cin.snap2.emplace(c.snap2Vec.value()[c3]);
            c3++;
            total++;
        }
        if (total)
        {
            coreInputsVec.push_back(cin);
        }
    }
    return coreInputsVec;
}


std::vector<MemorySnapshotSpan> makeSnapshotSpans(const std::vector<MemorySnapshot> &snapVec)
{
    std::vector<MemorySnapshotSpan> result;
    result.reserve(snapVec.size());
    for (const auto &snap: snapVec) {
        result.push_back(snap.asSnapshotSpan());
    }
    return result;
}

std::vector<MemorySnapshot> convertTasksIntoSnapshots(std::vector<Task<MemorySnapshot (*)(const CoreInputs &)>> &tasks)
{
    std::vector<MemorySnapshot> result;
    result.reserve(tasks.size());
    for (auto &task: tasks) {
        result.push_back(task.result.get());
    }
    return result;
}
};



namespace rmf::backends {
    std::vector<data::MemorySnapshot> MTBackend::makeSnapshots(
            const std::span<const data::MemoryRegionProperties>& rpl)
    {
        return p_doCoreFunc(core::makeSnapshotCore, std::views::zip(rpl));
        // auto tasksList = mt::initTasksList(core::makeSnapshotCore); 
        // for (const auto &mrp : rpl) {
        //     tasksList.push_back(mt::createTask(core::makeSnapshotCore, {.mrp = mrp}));
        // }
        // m_tp.submitMultipleTasks(tasksList);
        // m_tp.awaitAllTasks();
        // return mt::consolidateTasks(tasksList);
    }

    data::RegionPropertiesList MTBackend::findChangingRegions(
        const std::span<const data::MemorySnapshotSpan> span1,
        const std::span<const data::MemorySnapshotSpan> span2,
        const uintptr_t&                          compsize)
    {
        return p_doCoreFunc(core::findChangedRegionsCore, std::views::zip(span1, span2), compsize);
    }
    data::RegionPropertiesList MTBackend::findUnchangingRegions(
        const std::span<const data::MemorySnapshotSpan> span1,
        const std::span<const data::MemorySnapshotSpan> span2,
        const uintptr_t&                          compsize)
    {
        return p_doCoreFunc(core::findUnchangedRegionsCore, std::views::zip(span1, span2), compsize);
    };

    data::RegionPropertiesList MTBackend::findChangedNumeric(
        const std::span<const data::MemorySnapshotSpan> span1,
        const std::span<const data::MemorySnapshotSpan> span2,
        const data::NumQuery&                     query)
    {
        // return p_doCoreFunc(core::findUnchangedRegionsCore, std::views::zip(span1, span2), compsize);
    };

    data::RegionPropertiesList MTBackend::findUnchangedNumeric(
        const std::span<const data::MemorySnapshotSpan> span1,
        const std::span<const data::MemorySnapshotSpan> span2,
        const data::NumQuery&                     query)
    {
    };

    data::RegionPropertiesList
    MTBackend::findString(const std::span<const data::MemorySnapshotSpan> span1,
               const std::span<const data::MemorySnapshotSpan> span2,
               const std::string_view&                   strView) 
    {
        return p_doCoreFunc(core::findStringCore, std::views::zip(span1, span2), strView);
    };
}
