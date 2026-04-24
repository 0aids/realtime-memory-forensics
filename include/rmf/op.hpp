#ifndef op_hpp_INCLUDED
#define op_hpp_INCLUDED

#include "rmf/map.hpp"
#include "rmf/node.hpp"
#include "rmf/utils/function.hpp"
#include "rmf/utils/vec.hpp"
namespace RealtimeMemoryForensics
{
    namespace Detail
    {
        /******************************/
        /* Binary Snapshot Operations */
        /******************************/
        template <typename node_t>
        Utils::Vec<Node<Map>>
        findChanged(const node_t& snap1, const node_t& snap2,
                    const uintptr_t& compareSize);

        template <typename node_t>
        Utils::Vec<Node<Map>>
        findUnchanged(const node_t& snap1, const node_t& snap2,
                      const uintptr_t& compareSize);

        // Difference is calculated as snap2 - snap1
        // Inclusive.
        template <typename N, typename node_t>
        Utils::Vec<Node<Map>> findNumChanged(const node_t& snap1,
                                             const node_t& snap2,
                                             const N& minDifference);

        // Inclusive.
        template <typename N, typename node_t>
        Utils::Vec<Node<Map>>
        findNumUnchanged(const node_t& snap1, const node_t& snap2,
                         const N& maxDifference);

        /*****************************/
        /* Unary Snapshot Operations */
        /*****************************/

        template <typename node_t>
        Utils::Vec<Node<Map>> findString(const node_t&          snap1,
                                         const std::string_view str);

        template <typename N, typename node_t>
        Utils::Vec<Node<Map>> findNumExact(const node_t& snap1,
                                           const N       number);

        struct findNumExactShort
        {
            template <typename N, typename node_t>
            Utils::Vec<Node<Map>> operator()(node_t&& snap1,
                                             N&&      number);
        };

        // Inclusive.
        template <typename N, typename node_t>
        Utils::Vec<Node<Map>> findNumWithinRange(const node_t& snap1,
                                                 const N&      min,
                                                 const N&      max);
    }

    // Threadify functions.
    template <typename node_t>
    constexpr auto findChanged =
        Utils::Function(Detail::findChanged<node_t>);

    template <typename node_t>
    constexpr auto findUnchanged =
        Utils::Function(Detail::findUnchanged<node_t>);

    template <typename num_t, typename node_t>
    constexpr auto findNumChanged =
        Utils::Function(Detail::findNumChanged<num_t, node_t>);

    template <typename num_t, typename node_t>
    constexpr auto findNumUnchanged =
        Utils::Function(Detail::findNumUnchanged<num_t, node_t>);

    template <typename node_t>
    constexpr auto findString =
        Utils::Function(Detail::findString<node_t>);

    template <typename num_t, typename node_t>
    constexpr auto findNumExact =
        Utils::Function(Detail::findNumExact<num_t, node_t>);

    template <typename num_t, typename node_t>
    constexpr auto findNumWithinRange =
        Utils::Function(Detail::findNumWithinRange<num_t, node_t>);

    // TODO: Modify Utils::Function to allow having struct operator().
    // constexpr auto findNumExactShort =
    // 	Utils::Function(Detail::findNumExactShort{});

    Utils::Vec<Node<Map>> getMaps(pid_t pid);
}
namespace RealtimeMemoryForensics::Detail
{
    /******************************/
    /* Binary Snapshot Operations */
    /******************************/
    template <typename node_t>
    Utils::Vec<Node<Map>> findChanged(const node_t&    snap1,
                                      const node_t&    snap2,
                                      const uintptr_t& compareSize)
    {
    }

    template <typename node_t>
    Utils::Vec<Node<Map>> findUnchanged(const node_t&    snap1,
                                        const node_t&    snap2,
                                        const uintptr_t& compareSize)
    {
    }

    // Difference is calculated as snap2 - snap1
    // Inclusive.
    template <typename N, typename node_t>
    Utils::Vec<Node<Map>> findNumChanged(const node_t& snap1,
                                         const node_t& snap2,
                                         const N&      minDifference)
    {
    }

    // Inclusive.
    template <typename N, typename node_t>
    Utils::Vec<Node<Map>> findNumUnchanged(const node_t& snap1,
                                           const node_t& snap2,
                                           const N& maxDifference)
    {
    }

    /*****************************/
    /* Unary Snapshot Operations */
    /*****************************/

    template <typename node_t>
    Utils::Vec<Node<Map>> findString(const node_t&          snap1,
                                     const std::string_view str)
    {
    }

    template <typename N, typename node_t>
    Utils::Vec<Node<Map>> findNumExact(const node_t& snap1,
                                       const N       number)
    {
        auto span = snap1.span();
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
            N value;
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
    template <typename N, typename node_t>
    Utils::Vec<Node<Map>> findNumWithinRange(const node_t& snap1,
                                             const N&      min,
                                             const N&      max)
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
    template <typename N, typename node_t>
    Utils::Vec<Node<Map>>
    Detail::findNumExactShort::operator()(node_t&& snap1, N&& number)
    {
        return findNumExact<N, node_t>(std::forward<node_t>(snap1),
                                       std::forward<N>(number));
    }
}
#endif // op_hpp_INCLUDED
