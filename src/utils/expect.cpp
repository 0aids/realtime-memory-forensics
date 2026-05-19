#include "rmf/utils/expect.hpp"
#include <format>
#include <magic_enum/magic_enum.hpp>
namespace mf  = rmf;
namespace mfu = mf::Utils;

bool mfu::Error::hasError() const
{
    return err_what != ErrorEnum::Success;
}

const char* mfu::Error::whatError() const
{
    // I hope magic_enum strings are null-terminated.
    return magic_enum::enum_name(err_what).data();
}
