#ifndef snapshot_hpp_INCLUDED
#define snapshot_hpp_INCLUDED

#include "rmf/map.hpp"
#include <type_traits>

namespace RealtimeMemoryForensics
{
    using SnapshotBuffer = std::vector<uint8_t>;
    template <typename Base>
    class Snapshot
    {
      private:
        template <typename T>
        using sptr = std::shared_ptr<T>;
        sptr<SnapshotBuffer> m_data;
        // Ensure that we have data about maps.
      public:
        void wellFormed();
        void insertBuffer(SnapshotBuffer&& buffer);
        void capture(pid_t pid);
    };
}

namespace RealtimeMemoryForensics
{
    template <typename Base>
    void Snapshot<Base>::wellFormed()
    {
        static_assert(not std::is_base_of_v<Map<Base>, Base>,
                      "The base type must contain map!");
    }
}
#endif // snapshot_hpp_INCLUDED
