#ifndef op_hpp_INCLUDED
#define op_hpp_INCLUDED

#include <cstdint>
#include <cstring>
#include "rmf/map.hpp"
#include "rmf/node.hpp"
#include "rmf/snapshot.hpp"
#include "rmf/utils/function.hpp"

#include "rmf/utils/vec.hpp"
#include <type_traits>
#include <fstream>
namespace rmf
{
    namespace Detail
    {
        /******************************/
        /* Binary Snapshot Operations */
        /******************************/
        struct findChanged
        {
            template <typename node_t>
                requires IsNode<node_t>
            Utils::Vec<Node<Map>>
            operator()(const node_t& snap1, const node_t& snap2,
                       const uintptr_t& compareSize) const;
        };

        struct findUnchanged
        {
            template <typename node_t>
                requires IsNode<node_t>
            Utils::Vec<Node<Map>>
            operator()(const node_t& snap1, const node_t& snap2,
                       const uintptr_t& compareSize) const;
        };

        // Difference is calculated as snap2 - snap1
        // Inclusive.

        struct findNumChanged
        {
            template <typename node_t, typename N>
                requires IsNode<node_t>
            Utils::Vec<Node<Map>> operator()(const node_t& snap1,
                                             const node_t& snap2,
                                             const N&      minDifference) const;
        };

        // Inclusive.

        struct findNumUnchanged
        {
            template <typename node_t, typename N>
                requires IsNode<node_t>
            Utils::Vec<Node<Map>> operator()(const node_t& snap1,
                                             const node_t& snap2,
                                             const N&      maxDifference) const;
        };

        /*****************************/
        /* Unary Snapshot Operations */
        /*****************************/

        struct findString
        {
            template <typename node_t>
                requires IsNode<node_t>
            Utils::Vec<node_t> operator()(const node_t&          snap1,
                                          const std::string_view str) const;
        };

        struct findNumExact
        {
            template <typename... Features, typename N,
                      typename node_t = Node<Features...>>
                requires IsNode<node_t>
            Utils::Vec<node_t> operator()(const node_t& snap1,
                                          const N       number) const;
        };

        // Inclusive.

        struct findNumWithinRange
        {
            template <typename node_t, typename N>
                requires IsNode<node_t>
            Utils::Vec<Node<Map>> operator()(const node_t& snap1, const N& min,
                                             const N& max) const;
        };
        // struct captureNodes
        // {
        //     template <typename node_t>
        //         requires IsNode<node_t>
        //     void operator()(node_t& node, pid_t pid) const;
        // };
    }

    // Threadify functions.
    constexpr auto findChanged =
        Utils::Function<Detail::findChanged{}, Detail::findChanged{}, true>();

    constexpr auto findUnchanged =
        Utils::Function<Detail::findUnchanged{}, Detail::findUnchanged{},
                        true>();

    constexpr auto findNumChanged =
        Utils::Function<Detail::findNumChanged{}, Detail::findNumChanged{},
                        true>();

    constexpr auto findNumUnchanged =
        Utils::Function<Detail::findNumUnchanged{}, Detail::findNumUnchanged{},
                        true>();

    constexpr auto findString =
        Utils::Function<Detail::findString{}, Detail::findString{}, true>();
    // Functor version for pipe operations
    constexpr auto findStringF = [](const std::string& str) mutable
    { return [str](auto&& node) mutable { return findString(node, str); }; };

    constexpr auto findNumExact =
        Utils::Function<Detail::findNumExact{}, Detail::findNumExact{}, true>();

    constexpr auto findNumWithinRange =
        Utils::Function<Detail::findNumWithinRange{},
                        Detail::findNumWithinRange{}, true>();

    // constexpr auto captureNodes =
    //     Utils::Function<Detail::captureNodes{},
    //                     Detail::captureNodes{}, false>();

    template <typename... Features>
    Utils::Vec<Node<Map, Features...>> getMaps(pid_t pid);
}
namespace rmf::Detail
{
    /******************************/
    /* Binary Snapshot Operations */
    /******************************/
    template <typename node_t>
        requires IsNode<node_t>
    Utils::Vec<Node<Map>>
    findChanged::operator()(const node_t& nodeSnap1, const node_t& nodeSnap2,
                            const uintptr_t& compareSize) const
    {
        std::span<uint8_t>    span1 = nodeSnap1.span();
        std::span<uint8_t>    span2 = nodeSnap2.span();
        Node<Map>             mrp   = nodeSnap1;

        Utils::Vec<Node<Map>> results;
        uintptr_t             bytesCompared = 0;
        while (bytesCompared < span1.size())
        {
            uintptr_t toCompare = (span1.size() - bytesCompared > compareSize) ?
                                      compareSize :
                                      span1.size() - bytesCompared;

            if (memcmp(span1.data() + bytesCompared,
                       span2.data() + bytesCompared, toCompare))
            {
                // rmf_Debug("Found difference!");
                if (!results.empty() &&
                    results.back().rend() ==
                        mrp.map.relativeAddress + bytesCompared)
                {
                    results.back().map.relativeSize += toCompare;
                }
                else
                {
                    Node<Map> toPush = mrp;
                    toPush.map.relativeAddress += bytesCompared;
                    toPush.map.relativeSize = toCompare;
                    results.push_back(toPush);
                }
            }
            bytesCompared += toCompare;
        }
        rmf_Debug("Number of changed regions found: {}", results.size());
        return results;
    }

    template <typename node_t>
        requires IsNode<node_t>
    Utils::Vec<Node<Map>>
    findUnchanged::operator()(const node_t& nodeSnap1, const node_t& nodeSnap2,
                              const uintptr_t& compareSize) const
    {
        std::span<uint8_t>    span1 = nodeSnap1.span();
        std::span<uint8_t>    span2 = nodeSnap2.span();

        Utils::Vec<Node<Map>> results;
        uintptr_t             bytesCompared = 0;
        while (bytesCompared < span1.size())
        {
            uintptr_t toCompare = (span1.size() - bytesCompared > compareSize) ?
                                      compareSize :
                                      span1.size() - bytesCompared;

            if (!memcmp(span1.data() + bytesCompared,
                        span2.data() + bytesCompared, toCompare))
            {
                // rmf_Debug("Found No difference!");
                if (!results.empty() &&
                    results.back().rend() ==
                        nodeSnap1.map.relativeAddress + bytesCompared)
                {
                    results.back().map.relativeSize += toCompare;
                }
                else
                {
                    Node<Map> toPush = nodeSnap1;
                    toPush.map.relativeAddress += bytesCompared;
                    toPush.map.relativeSize = toCompare;
                    results.push_back(toPush);
                }
            }
            bytesCompared += toCompare;
        }
        rmf_Debug("Number of unchanged regions found: ", results.size());
        return results;
    }

    // Difference is calculated as snap2 - snap1
    // Inclusive.
    template <typename node_t, typename N>
        requires IsNode<node_t>
    Utils::Vec<Node<Map>>
    findNumChanged::operator()(const node_t& nodeSnap1, const node_t& nodeSnap2,
                               const N& minDifference) const
    {
        std::span<uint8_t>    span1 = nodeSnap1.span();
        std::span<uint8_t>    span2 = nodeSnap2.span();
        Utils::Vec<Node<Map>> results;
        const size_t          alignment = alignof(N);
        const size_t          size      = sizeof(N);

        // Prealign to the next available slot.
        uintptr_t bytesCompared = 0;
        if ((nodeSnap1.tbegin() / alignment) * alignment < nodeSnap1.tbegin())
        {
            bytesCompared += alignment +
                             (nodeSnap1.tbegin() / alignment * alignment) -
                             nodeSnap1.tbegin();
        }

        // Ensure we don't read out of bounds
        while (bytesCompared + size < span1.size())
        {
            N value1;
            memcpy(&value1, span1.data() + bytesCompared, size);
            N value2;
            memcpy(&value2, span2.data() + bytesCompared, size);

            N diff = value2 - value1;

            if (diff >= minDifference)
            {
                auto newnode = nodeSnap1;
                newnode.map.relativeAddress += bytesCompared;
                newnode.map.relativeSize = size;
                results.push_back(newnode);
            }

            bytesCompared += alignment;
        }

        rmf_Debug("Number of numerically changed regions found: {}",
                  results.size());
        return results;
    }

    // Inclusive.
    template <typename node_t, typename N>
        requires IsNode<node_t>
    Utils::Vec<Node<Map>>
    findNumUnchanged::operator()(const node_t& nodeSnap1,
                                 const node_t& nodeSnap2,
                                 const N&      maxDifference) const
    {
        std::span<uint8_t>    span1 = nodeSnap1.span();
        std::span<uint8_t>    span2 = nodeSnap2.span();
        Utils::Vec<Node<Map>> results;
        const size_t          alignment = alignof(N);
        const size_t          size      = sizeof(N);

        // Prealign to the next available slot.
        uintptr_t bytesCompared = 0;
        if ((nodeSnap1.tbegin() / alignment) * alignment < nodeSnap1.tbegin())
        {
            bytesCompared += alignment +
                             (nodeSnap1.tbegin() / alignment * alignment) -
                             nodeSnap1.tbegin();
        }

        // Ensure we don't read out of bounds
        while (bytesCompared + size < span1.size())
        {
            N value1;
            memcpy(&value1, span1.data() + bytesCompared, size);
            N value2;
            memcpy(&value2, span2.data() + bytesCompared, size);

            N diff = value2 - value1;

            if (diff <= maxDifference)
            {
                Node<Map> newmrp = nodeSnap1;
                newmrp.map.relativeAddress += bytesCompared;
                newmrp.map.relativeSize = size;
                results.push_back(newmrp);
            }

            bytesCompared += alignment;
        }

        rmf_Debug("Number of numerically unchanged regions found: {}",
                  results.size());
        return results;
    }

    /*****************************/
    /* Unary Snapshot Operations */
    /*****************************/

    template <typename node_t>
        requires IsNode<node_t>
    Utils::Vec<node_t> findString::operator()(const node_t&          nodeSnap,
                                              const std::string_view str) const
    {
        Utils::Vec<node_t> results;
        auto               span  = nodeSnap.span();
        const char*        head  = reinterpret_cast<const char*>(span.data());
        const char*        begin = head;
        const char*        end   = head + span.size();

        while (head < end)
        {
            head =
                static_cast<const char*>(std::memchr(head, str[0], end - head));
            if (!head)
                break;

            if (std::memcmp(head, str.data(), str.size()) == 0)
            {
                node_t mrp              = nodeSnap;
                mrp.map.relativeAddress = head - begin;
                mrp.map.relativeSize    = str.size();
                results.push_back(mrp);
            }
            head++;
        }

        return results;
    }

    template <typename... Features, typename N, typename node_t>
        requires IsNode<node_t>
    Utils::Vec<node_t> findNumExact::operator()(const node_t& nodeSnap,
                                                const N       number) const
    {
        using num_t             = std::decay_t<N>;
        std::span<uint8_t> span = nodeSnap.span();
        // Convert wider node into thinner node.
        Utils::Vec<node_t> results;
        const size_t       alignment     = alignof(N);
        const size_t       size          = sizeof(N);
        uintptr_t          bytesCompared = 0;
        if ((nodeSnap.tbegin() / alignment) * alignment < nodeSnap.tend())
        {
            bytesCompared += alignment +
                             (nodeSnap.tbegin() / alignment * alignment) -
                             nodeSnap.tbegin();
        }

        while (bytesCompared + size < span.size())
        {
            num_t value;
            memcpy(&value, span.data() + bytesCompared, size);
            if (value == number)
            {
                node_t node = nodeSnap;
                node.map.relativeAddress += bytesCompared;
                node.map.relativeSize = size;
                results.push_back(node);
            }
            bytesCompared += alignment;
        }
        return results;
    }

    // Inclusive.
    template <typename node_t, typename N>
        requires IsNode<node_t>
    Utils::Vec<Node<Map>> findNumWithinRange::operator()(const node_t& nodeSnap,
                                                         const N&      min,
                                                         const N& max) const
    {
        std::span<uint8_t>    span = nodeSnap.span();
        Utils::Vec<Node<Map>> results;
        const size_t          alignment     = alignof(N);
        const size_t          size          = sizeof(N);
        uintptr_t             bytesCompared = 0;
        if ((nodeSnap.tbegin() / alignment) * alignment < nodeSnap.tbegin())
        {
            bytesCompared += alignment +
                             (nodeSnap.tbegin() / alignment * alignment) -
                             nodeSnap.tbegin();
        }

        while (bytesCompared + size < span.size())
        {
            N value;
            memcpy(&value, span.data() + bytesCompared, size);
            if (min <= value && value <= max)
            {
                Node<Map> node = nodeSnap;
                node.map.relativeAddress += bytesCompared;
                node.map.relativeSize = size;
                results.push_back(node);
            }
            bytesCompared += alignment;
        }
        return results;
    }

    // Boilerplate, but can't find any other way to do it.
    // Explicit object param + templates is not compatible with
    // being threaded, nor compatible with being passed as function
    // parameters.
    // template <typename node_t>
    //     requires IsNode<node_t>
    // void captureNodes::operator()(node_t& node, pid_t pid) const
    // {
    //     node.capture(pid);
    // }

}
namespace rmf
{
    template <typename... Features>
    Utils::Vec<Node<Map, Features...>> getMaps(pid_t pid)
    {
        using NodeBase = Node<Map, Features...>;
        using namespace Utils::Literals;
        std::ifstream        memoryMapFile("/proc/{}/maps"_f.fmt(pid));
        std::string          line;
        int                  unnamedRegionNumber = 1;

        Utils::Vec<NodeBase> regionProperties;

        while (std::getline(memoryMapFile, line))
        {
            uintptr_t   startAddr, endAddr;
            std::string perms;
            perms.resize(4, '-');
            std::string name;
            name.resize(1024, 0);
            sscanf(line.c_str(), "%lx-%lx %c%c%c%c %*s %*s %*s %[^\n]",
                   &startAddr, &endAddr, &perms[0], &perms[1], &perms[2],
                   &perms[3], name.data());
            name.resize(strlen(name.c_str()));

            if (name.size() == 0)
            {
                name = "UnnamedRegion-" + std::to_string(unnamedRegionNumber++);
            }

            Detail::MapData m = {
                .parentAddress   = startAddr,
                .parentSize      = endAddr - startAddr,
                .relativeAddress = 0,
                .relativeSize    = static_cast<ptrdiff_t>(endAddr - startAddr),
                .regionName_sp   = std::make_shared<const std::string>(name),
                .perms           = Detail::parsePerms(perms),
            };
            NodeBase node;
            node.map = m;
            regionProperties.push_back(node);
        }
        return regionProperties;
    }
}
#endif // op_hpp_INCLUDED
