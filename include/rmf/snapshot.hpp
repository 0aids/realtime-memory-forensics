#ifndef snapshot_hpp_INCLUDED
#define snapshot_hpp_INCLUDED

#include "rmf/map.hpp"
#include <cstddef>
#include <type_traits>

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
    template <typename Base>
    class Snapshot
    {
      private:
        Detail::SnapshotData snap;

      public:
        using usesSnapshot = std::true_type;
        // Factory methods for instantiation.

        static Base makeSnapshot(pid_t pid, Base b);
        static Base fromBuffer(SnapshotBuffer&& b);

        // Does nothing, but just ensures that instantiation
        // is valid during comptime, as static_asserts do
        // not work during class type defining time, as this class
        // would not be fully instantiated.
        void wellFormed();

        // For testing, allow inserting other buffers that we generate ourselves.
        void insertBuffer(SnapshotBuffer&& buffer);
        void insertBuffer(const SnapshotBuffer& buffer);

        // Creates another capture.
        void capture(pid_t pid);
        // Returns a view into the data.
        std::span<uint8_t> span() const;
    };
}

namespace RealtimeMemoryForensics
{
    template <typename Base>
    Base Snapshot<Base>::fromBuffer(SnapshotBuffer&& b)
    {
        Base node;
        node.snap.m_data =
            std::make_shared<SnapshotBuffer>(std::move(b));
        node.map.parentSize   = node.snap.m_data->size();
        node.map.relativeSize = node.snap.m_data->size();
        return node;
    }
    // Implement functions to capture.
    template <typename Base>
    Base Snapshot<Base>::makeSnapshot(pid_t pid, Base b)
    {
        constexpr ptrdiff_t chunkSize = 1 << 24;
        Base                node(b);
        Node<Map>           mrp(b);

        struct iovec        localIovec[1];
        struct iovec        sourceIovec[1];

        node.snap.m_data->resize(b.relativeAddress);
        intptr_t totalBytesRead = 0;
        while (totalBytesRead <
               static_cast<intptr_t>(mrp.map.relativeSize))
        {
            uintptr_t bytesToRead =
                (mrp.map.relativeSize - totalBytesRead > chunkSize) ?
                chunkSize :
                mrp.map.relativeSize - totalBytesRead;

            sourceIovec[0].iov_base =
                (void*)(mrp.tbegin() + totalBytesRead);
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
                    //                 << mrp.relativeRegionSize
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
    template <typename Base>
    void Snapshot<Base>::wellFormed()
    {
        static_assert(not std::is_base_of_v<Map<Base>, Base>,
                      "The base type must contain map!");
    }

    template <typename Base>
    std::span<uint8_t> Snapshot<Base>::span() const
    { return *snap.m_data; }
}
#endif // snapshot_hpp_INCLUDED
