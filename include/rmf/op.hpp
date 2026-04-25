#ifndef op_hpp_INCLUDED
#define op_hpp_INCLUDED

#include "rmf/map.hpp"
#include "rmf/node.hpp"
#include "rmf/utils/function.hpp"
#include "rmf/utils/vec.hpp"
#include <type_traits>
namespace RealtimeMemoryForensics
{
    namespace Detail
    {
        /******************************/
        /* Binary Snapshot Operations */
        /******************************/
        struct findChanged
        {
            template <typename node_t>
            Utils::Vec<Node<Map>>
            operator()(const node_t& snap1, const node_t& snap2,
                       const uintptr_t& compareSize) const;
        };

        struct findUnchanged
        {
            template <typename node_t>
            Utils::Vec<Node<Map>>
            operator()(const node_t& snap1, const node_t& snap2,
                       const uintptr_t& compareSize) const;
        };

        // Difference is calculated as snap2 - snap1
        // Inclusive.

        struct findNumChanged
        {
            template <typename node_t, typename N>
            Utils::Vec<Node<Map>>
            operator()(const node_t& snap1, const node_t& snap2,
                       const N& minDifference) const;
        };

        // Inclusive.

        struct findNumUnchanged
        {
            template <typename node_t, typename N>
            Utils::Vec<Node<Map>>
            operator()(const node_t& snap1, const node_t& snap2,
                       const N& maxDifference) const;
        };

        /*****************************/
        /* Unary Snapshot Operations */
        /*****************************/

        struct findString
        {
            template <typename node_t>
            Utils::Vec<Node<Map>>
            operator()(const node_t&          snap1,
                       const std::string_view str) const;
        };

        struct findNumExact
        {
            template <template <typename> typename... Features,
                      typename N, typename node_t = Node<Features...>>
                requires IsNode<node_t>
            Utils::Vec<Node<Map>> operator()(const node_t& snap1,
                                             const N number) const;
        };

        // Inclusive.

        struct findNumWithinRange
        {
            template <typename node_t, typename N>
            Utils::Vec<Node<Map>> operator()(const node_t& snap1,
                                             const N&      min,
                                             const N&      max) const;
        };
    }

    // problemo - How the fuck do i get filtering to work?
    template <template <typename> typename... Features>
    Node<Map, Features...> getMapsFromPid(pid_t pid);

    // Threadify functions.
    constexpr auto findChanged =
        Utils::Function(Detail::findChanged{});

    constexpr auto findUnchanged =
        Utils::Function(Detail::findUnchanged{});

    constexpr auto findNumChanged =
        Utils::Function(Detail::findNumChanged{});

    constexpr auto findNumUnchanged =
        Utils::Function(Detail::findNumUnchanged{});

    constexpr auto findString = Utils::Function(Detail::findString{});

    constexpr auto findNumExact =
        Utils::Function(Detail::findNumExact{});

    constexpr auto findNumWithinRange =
        Utils::Function(Detail::findNumWithinRange{});

    Utils::Vec<Node<Map>> getMaps(pid_t pid);
}
namespace RealtimeMemoryForensics::Detail
{
    /******************************/
    /* Binary Snapshot Operations */
    /******************************/
    template <typename node_t>
    Utils::Vec<Node<Map>>
    findChanged::operator()(const node_t& snap1, const node_t& snap2,
                            const uintptr_t& compareSize) const
    {
    }

    template <typename node_t>
    Utils::Vec<Node<Map>>
    findUnchanged::operator()(const node_t&    snap1,
                              const node_t&    snap2,
                              const uintptr_t& compareSize) const
    {
    }

    // Difference is calculated as snap2 - snap1
    // Inclusive.
    template <typename node_t, typename N>
    Utils::Vec<Node<Map>>
    findNumChanged::operator()(const node_t& snap1,
                               const node_t& snap2,
                               const N&      minDifference) const
    {
    }

    // Inclusive.
    template <typename node_t, typename N>
    Utils::Vec<Node<Map>>
    findNumUnchanged::operator()(const node_t& snap1,
                                 const node_t& snap2,
                                 const N&      maxDifference) const
    {
    }

    /*****************************/
    /* Unary Snapshot Operations */
    /*****************************/

    template <typename node_t>
    Utils::Vec<Node<Map>>
    findString::operator()(const node_t&          snap1,
                           const std::string_view str) const
    {
    }

    template <template <typename> typename... Features, typename N,
              typename node_t>
        requires IsNode<node_t>
    Utils::Vec<Node<Map>>
    findNumExact::operator()(const node_t& snap1,
                             const N       number) const
    {
        using num_t = std::decay_t<N>;
        auto span   = snap1.span();
        // Convert wider node into thinner node.
        auto                  mrp = static_cast<Node<Map>>(snap1);
        Utils::Vec<Node<Map>> results;
        const size_t          alignment     = alignof(N);
        const size_t          size          = sizeof(N);
        uintptr_t             bytesCompared = 0;
        if ((mrp.tbegin() / alignment) * alignment < mrp.tend())
        {
            bytesCompared += alignment +
                (mrp.tbegin() / alignment * alignment) - mrp.tbegin();
        }

        while (bytesCompared + size < span.size())
        {
            num_t value;
            memcpy(&value, span.data() + bytesCompared, size);
            if (value == number)
            {
                auto newmrp = mrp;
                newmrp.map.relativeAddress += bytesCompared;
                newmrp.map.relativeSize = size;
                results.push_back(newmrp);
            }
            bytesCompared += alignment;
        }
        return results;
    }

    // Inclusive.
    template <typename node_t, typename N>
    Utils::Vec<Node<Map>>
    findNumWithinRange::operator()(const node_t& snap1, const N& min,
                                   const N& max) const
    {
        auto                  span = snap1.span();
        const Node<Map>       mrp  = snap1;
        Utils::Vec<Node<Map>> results;
        const size_t          alignment     = alignof(N);
        const size_t          size          = sizeof(N);
        uintptr_t             bytesCompared = 0;
        if ((mrp.tbegin() / alignment) * alignment < mrp.tbegin())
        {
            bytesCompared += alignment +
                (mrp.tbegin() / alignment * alignment) - mrp.tbegin();
        }

        while (bytesCompared + size < span.size())
        {
            N value;
            memcpy(&value, span.data() + bytesCompared, size);
            if (min <= value && value <= max)
            {
                auto newmrp = mrp;
                newmrp.map.relativeAddress += bytesCompared;
                newmrp.map.relativeSize = size;
                results.push_back(newmrp);
            }
            bytesCompared += alignment;
        }
        return results;
    }
}
#endif // op_hpp_INCLUDED
