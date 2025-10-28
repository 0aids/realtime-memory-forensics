#include "core_wrappers.hpp"

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

std::vector<std::span<const char>> divideSingleSnapshot(const MemorySnapshot &snap, size_t quantity) {
    std::vector<std::span<const char>> spanVec;
    size_t avgSize = snap.size() / quantity;

    for (size_t i = 0; i < quantity; i++)
    {
        size_t numBytes = (i == quantity - 1) ? snap.size() - avgSize * i :
            avgSize;
        size_t offset = 
            avgSize * i;
        spanVec.push_back(snap.asSpan().subspan(
            offset,
            numBytes
        ));
        Log(Debug, std::hex << std::showbase <<
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
            cin.snap1 = c.snap1Vec.value()[c2];
            c2++;
            total++;
        }
        if (c.snap2Vec && c3 < c.snap2Vec.value().size()) {
            cin.snap2 = c.snap2Vec.value()[c3];
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


std::vector<std::span<const char>> divideMultipleSnapshots(const std::vector<MemorySnapshot> &snapVec)
{

    std::vector<std::span<const char>> result;
    result.reserve(snapVec.size());
    for (const auto &snap: snapVec) {
        result.push_back(snap.asSpan());
    }
    return result;
}
