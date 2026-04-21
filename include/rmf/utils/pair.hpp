#ifndef pair_hpp_INCLUDED
#define pair_hpp_INCLUDED
#include <tuple>
namespace RealtimeMemoryForensics::Utils
{
    template <typename... Elements>
    using tuple = std::tuple<Elements...>;
}

#endif // pair_hpp_INCLUDED
