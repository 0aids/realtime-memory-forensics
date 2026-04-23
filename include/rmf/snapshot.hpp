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
        uint64_t             timestamp;
        // Ensure that we have data about maps.
      public:
        using usesSnapshot = std::true_type;

        void               wellFormed();
        void               insertBuffer(SnapshotBuffer&& buffer);
        void               capture(pid_t pid);
        std::span<uint8_t> getSpan();
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

    template <typename Base>
    std::span<uint8_t> Snapshot<Base>::getSpan()
    { return *m_data; }
}
#endif // snapshot_hpp_INCLUDED
