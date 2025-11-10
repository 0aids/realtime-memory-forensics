#ifndef ibackend_hpp_INCLUDED
#define ibackend_hpp_INCLUDED
#include <cstdint>
#include <ranges>
#include <string_view>
#include "../data/maps.hpp"
#include "../data/snapshots.hpp"
#include "../data/numerics.hpp"
#include "mt_backend.hpp"
#include "core.hpp"

namespace rmf::backends
{
    using namespace rmf;
    class IBackend
    {
      public:
        virtual ~IBackend() = default;

        // In non-multithreaded contexts this can do nothing.
        virtual void setChunkSize(const uintptr_t size) = 0;

        virtual std::vector<data::MemorySnapshot> makeSnapshots(
            const std::span<const data::MemoryRegionProperties>& rpl) = 0;

        virtual data::RegionPropertiesList findChangingRegions(
            const std::span<const data::MemorySnapshotSpan> span1,
            const std::span<const data::MemorySnapshotSpan> span2,
            const uintptr_t&                          compsize) = 0;

        virtual data::RegionPropertiesList findUnchangingRegions(
            const std::span<const data::MemorySnapshotSpan> span1,
            const std::span<const data::MemorySnapshotSpan> span2,
            const uintptr_t&                          compsize) = 0;

        virtual data::RegionPropertiesList findChangedNumeric(
            const std::span<const data::MemorySnapshotSpan> span1,
            const std::span<const data::MemorySnapshotSpan> span2,
            const data::NumQuery&                     query) = 0;

        virtual data::RegionPropertiesList findUnchangedNumeric(
            const std::span<const data::MemorySnapshotSpan> span1,
            const std::span<const data::MemorySnapshotSpan> span2,
            const data::NumQuery&                     query) = 0;

        virtual data::RegionPropertiesList
        findString(std::span<const data::MemorySnapshotSpan> span1,
                   std::span<const data::MemorySnapshotSpan> span2,
                   const std::string_view&                   strView) = 0;
    };

    class MTBackend : IBackend
    {
      private:
        mt::QueuedThreadPool m_tp;
        uintptr_t chunkSize = UINTPTR_MAX; 

        template <typename CoreFuncType, typename... CoreFuncInputs, typename... ExtraInputs>
        // std::vector<std::invoke_result_t<CoreFuncType>> 
        auto p_doCoreFunc(CoreFuncType coreFunc, std::ranges::zip_view<CoreFuncInputs...> inputs, ExtraInputs... extraInputs) {
            using ResultType = mt::Task<CoreFuncType>::ReturnType;

            auto tasksList = mt::initTasksList(coreFunc);
            for (const auto &tuple : inputs) {
                core::CoreInputs cinputs = std::apply([](auto&&... args) {
                    return core::CoreInputs(std::forward<decltype(args)>(args)...);
                }, tuple);
                tasksList.push_back(mt::createTask(coreFunc, cinputs, extraInputs...));
            }
            m_tp.submitMultipleTasks(tasksList);
            m_tp.awaitAllTasks();
            if constexpr (mt::isContainer<ResultType>)
            {
                return mt::consolidateTasksNestedResults(tasksList) ;
            }
            else
                return mt::consolidateTasks(tasksList);
        }

      public:
        MTBackend(size_t numThreads) : m_tp(numThreads) {}

        void setChunkSize(const uintptr_t size) override{};

        std::vector<data::MemorySnapshot> makeSnapshots(
            const std::span<const data::MemoryRegionProperties>& rpl)
            override;

        data::RegionPropertiesList findChangingRegions(
            const std::span<const data::MemorySnapshotSpan> span1,
            const std::span<const data::MemorySnapshotSpan> span2,
            const uintptr_t&                          compsize) override;

        data::RegionPropertiesList findUnchangingRegions(
            const std::span<const data::MemorySnapshotSpan> span1,
            const std::span<const data::MemorySnapshotSpan> span2,
            const uintptr_t&                          compsize) override;

        data::RegionPropertiesList findChangedNumeric(
            const std::span<const data::MemorySnapshotSpan> span1,
            const std::span<const data::MemorySnapshotSpan> span2,
            const data::NumQuery&                     query) override;

        data::RegionPropertiesList findUnchangedNumeric(
            const std::span<const data::MemorySnapshotSpan> span1,
            const std::span<const data::MemorySnapshotSpan> span2,
            const data::NumQuery&                     query) override;

        data::RegionPropertiesList
        findString(const std::span<const data::MemorySnapshotSpan> span1,
                   const std::span<const data::MemorySnapshotSpan> span2,
                   const std::string_view&                   strView) override;
    };
}

#endif // ibackend_hpp_INCLUDED
