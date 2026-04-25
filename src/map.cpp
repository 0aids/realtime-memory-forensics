#include "rmf/map.hpp"
namespace mf  = RealtimeMemoryForensics;
namespace mfd = mf::Detail;
namespace RealtimeMemoryForensics
{
    namespace Detail
    {
        std::shared_ptr<const std::string> defaultName =
            std::make_shared<const std::string>("");

        Perms parsePerms(const std::string_view perms)
        {
            using namespace magic_enum::bitwise_operators;
            Perms ps = Perms::None;
            if (perms.contains('r'))
            {
                ps |= Perms::Read;
            }
            if (perms.contains('w'))
            {
                ps |= Perms::Write;
            }
            if (perms.contains('x'))
            {
                ps |= Perms::Execute;
            }
            if (perms.contains('s'))
            {
                ps |= Perms::Shared;
            }
            return ps;
        }
    }

    std::shared_ptr<const std::string> Detail::MapData::defaultName =
        Detail::defaultName;
}
