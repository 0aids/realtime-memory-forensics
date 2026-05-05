#include "rmf/snapshot.hpp"
#include "rmf/map.hpp"
#include "rmf/node.hpp"
#include "rmf/utils/function.hpp"
#include <type_traits>

namespace RealtimeMemoryForensics
{
    Snapshot::operator std::string() const
    { return ""; }

    namespace Detail
    {
        SnapshotData readProcess(const MapData& map, pid_t pid)
        {
            constexpr ptrdiff_t chunkSize = 1 << 24;

            struct iovec        localIovec[1];
            struct iovec        sourceIovec[1];
            Node<Map>           b;
            b.map = map;

            SnapshotData data = {
                .m_data = std::make_shared<SnapshotBuffer>()};

            data.m_data->resize(b.map.relativeSize);
            intptr_t totalBytesRead = 0;
            while (totalBytesRead <
                   static_cast<intptr_t>(b.map.relativeSize))
            {
                uintptr_t bytesToRead =
                    (b.map.relativeSize - totalBytesRead >
                     chunkSize) ?
                    chunkSize :
                    b.map.relativeSize - totalBytesRead;

                sourceIovec[0].iov_base =
                    (void*)(b.tbegin() + totalBytesRead);
                sourceIovec[0].iov_len = bytesToRead;

                localIovec[0].iov_base =
                    data.m_data->data() + totalBytesRead;
                localIovec[0].iov_len = bytesToRead;

                ssize_t nread = process_vm_readv(pid, localIovec, 1,
                                                 sourceIovec, 1, 0);

                if (nread <= 0)
                {
                    if (nread == -1 && totalBytesRead > 0)
                    {
                        data.m_data->clear();
                        // rmf_Log(rmf_Error,
                        //         "Read " << nread << "/"
                        //                 << b.relativeRegionSize
                        //                 << "bytes. Failed to read "
                        //                    "all the bytes "
                        //                    "from that region.");
                        // rmf_Log(rmf_Error, "Error is below: ");
                        perror("process_vm_readv");
                        return data;
                    }
                    // rmf_Log(rmf_Error,
                    //         "Completely failed to read the region. "
                    //         "Error is below");
                    perror("process_vm_readv");
                    data.m_data->resize(totalBytesRead);
                    return data;
                }
                totalBytesRead += nread;
            }
            return data;
        }
    }
}
