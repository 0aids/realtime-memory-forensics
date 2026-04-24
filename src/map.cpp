#include "rmf/map.hpp"
namespace mf  = RealtimeMemoryForensics;
namespace mfd = mf::Detail;
namespace RealtimeMemoryForensics
{
    namespace Detail
    {
        std::shared_ptr<const std::string> defaultName =
            std::make_shared<const std::string>("");
    }

    std::shared_ptr<const std::string> Detail::MapData::defaultName =
        Detail::defaultName;
}
