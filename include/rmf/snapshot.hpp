#ifndef snapshot_hpp_INCLUDED
#define snapshot_hpp_INCLUDED

#include "rmf/map.hpp"
#include "rmf/mixin_helpers.hpp"
#include "rmf/utils/function.hpp"
#include <cstddef>
#include <type_traits>
#include <span>

namespace rmf
{
    using SnapshotBuffer = std::vector<uint8_t>;
    namespace Detail
    {
        struct SnapshotData
        {
            template <typename T>
            using sptr                  = std::shared_ptr<T>;
            sptr<SnapshotBuffer> m_data = nullptr;
            // Ensure that we have data about maps.
        };
        SnapshotData readProcess(const MapData& map, pid_t pid);
    }
    // We don't need to ensure that the buffer inside the snapshot
    // has to be aligned perfectly the same as the others.
    class Snapshot
    {
      private:
        Detail::SnapshotData snap;

      public:
        struct VecOp
        {
            // template <class vecSelf>
            // void capture(this vecSelf& self, pid_t pid);
        };
        using usesSnapshot = std::true_type;

        // Factory methods for instantiation.

        template <class... Features, class Other>
        static Node<Map, Snapshot, Features...> makeSnapshot(pid_t pid,
                                                             Other b);

        template <class... Features>
        static Node<Map, Snapshot, Features...> fromBuffer(SnapshotBuffer&& b);

        // Does nothing, but just ensures that instantiation
        // is valid during comptime, as static_asserts do
        // not work during class type defining time, as this class
        // would not be fully instantiated.
        template <class Self>
        void wellFormed(this const Self& self);

        // For testing, allow inserting other buffers that we generate ourselves.
        template <class Self>
        void insertBuffer(this Self& self, SnapshotBuffer&& buffer);

        template <class Self>
        void insertBuffer(this Self& self, const SnapshotBuffer& buffer);

        // Creates another capture.
        // It creates a shallow copy, as in the snapshot itself
        // is modified on both.
        // Consider making this a pure function.
        template <class Self>
        Self RMF_MIXIN_METHOD(capture, (this Self & self, pid_t pid));

        // Returns a view into the data.
        template <class Self>
        std::span<uint8_t> span(this const Self& self);

                           operator std::string() const;
    };
}

namespace rmf
{

    template <class... Features>
    Node<Map, Snapshot, Features...> Snapshot::fromBuffer(SnapshotBuffer&& b)
    {
        Node<Map, Snapshot, Features...> node;
        node.snap.m_data      = std::make_shared<SnapshotBuffer>(b);
        node.map.parentSize   = node.snap.m_data->size();
        node.map.relativeSize = node.snap.m_data->size();
        return node;
    }

    template <class... Features, class Other>
    Node<Map, Snapshot, Features...> Snapshot::makeSnapshot(pid_t pid, Other b)
    {
        // rmf_Log(rmf_Debug,
        //         "Time taken for snapshot: "
        //             << std::chrono::duration_cast<
        //                    std::chrono::milliseconds>(after -
        //                                               before));
        Node<Map, Snapshot, Features...> node;
        node.map  = b.map;
        node.snap = Detail::readProcess(b.map, pid);

        return node;
    }

    template <class Self>
    void Snapshot::wellFormed(this const Self&)
    {
        static_assert(not NodeWithFeatures<Self, Map>,
                      "The base type must contain map!");
    }

    template <class Self>
    std::span<uint8_t> Snapshot::span(this const Self& self)
    {
        return *self.snap.m_data;
    }

    template <class Self>
    Self Snapshot::capture(this Self& self, pid_t pid)
    {
        self.snap = Detail::readProcess(self.map, pid);
        return self;
    }

    // template <class vecSelf>
    // void Snapshot::VecOp::capture(this vecSelf& self, pid_t pid)
    // {
    //     for (auto& node : self)
    //     {
    //         node.capture(pid);
    //     }
    // }

}
#endif // snapshot_hpp_INCLUDED
