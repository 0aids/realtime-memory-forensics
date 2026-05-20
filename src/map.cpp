#include "rmf/map.hpp"
#include <magic_enum/magic_enum_flags.hpp>
namespace mf  = rmf;
namespace mfd = mf::Detail;
namespace rmf
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

    // Debugging use?
    Map::operator std::string()
    {
        return std::format(
            "\"{}\" - parent: [{:p}, {:p}) actual: [{:p}, {:p}) perms: {}",
            *map.regionName_sp, (void*)pbegin(), (void*)pend(), (void*)tbegin(),
            (void*)tend(), magic_enum::enum_flags_name(map.perms));
    }
}
