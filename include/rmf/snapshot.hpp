#ifndef snapshot_hpp_INCLUDED
#define snapshot_hpp_INCLUDED

#include "rmf/map.hpp"
#include <cstddef>
#include <type_traits>
#include <span>

namespace RealtimeMemoryForensics
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
        };
        using usesSnapshot = std::true_type;

        // Factory methods for instantiation.

        template <class... Features, class Other>
        static Node<Map, Snapshot, Features...>
        makeSnapshot(pid_t pid, Other b);

        template <class... Features>
        static Node<Map, Snapshot, Features...>
        fromBuffer(SnapshotBuffer&& b);

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
        void insertBuffer(this Self&            self,
                          const SnapshotBuffer& buffer);

        // Creates another capture.
        template <class Self>
        void capture(this Self& self, pid_t pid);
        // Returns a view into the data.
        template <class Self>
        std::span<uint8_t> span(this const Self& self);
    };
}

namespace RealtimeMemoryForensics
{

    template <class... Features>
    Node<Map, Snapshot, Features...>
    Snapshot::fromBuffer(SnapshotBuffer&& b)
    {
        Node<Map, Snapshot, Features...> node;
        node.snap.m_data      = std::make_shared<SnapshotBuffer>(b);
        node.map.parentSize   = node.snap.m_data->size();
        node.map.relativeSize = node.snap.m_data->size();
        return node;
    }

    template <class... Features, class Other>
    Node<Map, Snapshot, Features...> Snapshot::makeSnapshot(pid_t pid,
                                                            Other b)
    {
        constexpr ptrdiff_t              chunkSize = 1 << 24;
        Node<Map, Snapshot, Features...> node(b);

        struct iovec                     localIovec[1];
        struct iovec                     sourceIovec[1];

        node.snap.m_data->resize(b.relativeAddress);
        intptr_t totalBytesRead = 0;
        while (totalBytesRead <
               static_cast<intptr_t>(b.map.relativeSize))
        {
            uintptr_t bytesToRead =
                (b.map.relativeSize - totalBytesRead > chunkSize) ?
                chunkSize :
                b.map.relativeSize - totalBytesRead;

            sourceIovec[0].iov_base =
                (void*)(b.tbegin() + totalBytesRead);
            sourceIovec[0].iov_len = bytesToRead;

            localIovec[0].iov_base =
                node.snap.m_data->data() + totalBytesRead;
            localIovec[0].iov_len = bytesToRead;

            ssize_t nread = process_vm_readv(pid, localIovec, 1,
                                             sourceIovec, 1, 0);

            if (nread <= 0)
            {
                if (nread == -1 && totalBytesRead > 0)
                {
                    node.snap.m_data->clear();
                    // rmf_Log(rmf_Error,
                    //         "Read " << nread << "/"
                    //                 << b.relativeRegionSize
                    //                 << "bytes. Failed to read "
                    //                    "all the bytes "
                    //                    "from that region.");
                    // rmf_Log(rmf_Error, "Error is below: ");
                    perror("process_vm_readv");
                    return node;
                }
                // rmf_Log(rmf_Error,
                //         "Completely failed to read the region. "
                //         "Error is below");
                perror("process_vm_readv");
                node.snap.m_data->resize(totalBytesRead);
                return node;
            }
            totalBytesRead += nread;
        }
        // rmf_Log(rmf_Debug,
        //         "Time taken for snapshot: "
        //             << std::chrono::duration_cast<
        //                    std::chrono::milliseconds>(after -
        //                                               before));

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
    { return *self.snap.m_data; }
}
#endif // snapshot_hpp_INCLUDED
