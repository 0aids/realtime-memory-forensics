#ifndef snapshot_hpp_INCLUDED
#define snapshot_hpp_INCLUDED

#include "rmf/map.hpp"
#include <type_traits>
namespace RealtimeMemoryForensics
{
    template <typename Base>
    class Snapshot
    {
        // Ensure that we have data about maps.
      public:
        using WellFormed = std::true_type;
        void wellFormed()
        {
            static_assert(std::is_base_of_v<Map<Base>, Base>,
                          "please assert!");
        }
    };
}

#endif // snapshot_hpp_INCLUDED
