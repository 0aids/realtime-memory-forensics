#ifndef op_hpp_INCLUDED
#define op_hpp_INCLUDED

#include <cstdint>
#include <cstring>
#include "rmf/map.hpp"
#include "rmf/node.hpp"
#include "rmf/snapshot.hpp"
#include "rmf/utils/meta.hpp"
#include "rmf/utils/vec.hpp"
#include <iterator>
#include <string_view>
#include <type_traits>
#include <fstream>
namespace rmf
{
    template <typename T>
    concept OpCompatible = NodeWithFeatures<Map, Snapshot>;
    // All node operations return a node with the same features as the first node inputted.
    // All nodes can be implicitly converted from eachother so no problems shoud happen anyways.
    // First thing you'll probably need to use is this to get all maps.
    // By default it will only give you a node with a map.
    template <typename... Features>
    Utils::Vec<Node<Map, Features...>> getMaps(pid_t pid);

    /******************************/
    /* Binary Snapshot Operations */
    /******************************/

    template <OpCompatible node1_t, OpCompatible node2_t,
              OpCompatible nodeR_t = node1_t>
    Utils::Vec<nodeR_t> findChanged(const node1_t& snap1, const node2_t& snap2,
                                    const uintptr_t& compareSize);

    template <OpCompatible node1_t, OpCompatible node2_t,
              OpCompatible nodeR_t = node1_t>
    Utils::Vec<nodeR_t> findUnchanged(const node1_t&   snap1,
                                      const node2_t&   snap2,
                                      const uintptr_t& compareSize);

    // Difference is calculated as snap2 - snap1
    // Inclusive.

    template <OpCompatible node1_t, OpCompatible node2_t, Meta::Numeric N,
              OpCompatible nodeR_t = node1_t>
    Utils::Vec<nodeR_t> findNumChanged(const node1_t& snap1,
                                       const node2_t& snap2,
                                       const N&       minDifference);

    // Inclusive.

    template <OpCompatible node1_t, OpCompatible node2_t, Meta::Numeric N,
              OpCompatible nodeR_t = node1_t>
    Utils::Vec<nodeR_t> findNumUnchanged(const node1_t& snap1,
                                         const node2_t& snap2,
                                         const N&       maxDifference);

    /*****************************/
    /* Unary Snapshot Operations */
    /*****************************/

    template <OpCompatible node_t, OpCompatible nodeR_t = node_t>
    Utils::Vec<nodeR_t> findString(const node_t&          snap1,
                                   const std::string_view str);

    constexpr auto      findStringF = [](const std::string_view str)
    {
        return [str]<OpCompatible node_t>(const node_t& snap1) mutable
        { return findString(snap1, str); };
    };

    template <OpCompatible node_t, Meta::Numeric N,
              OpCompatible nodeR_t = node_t>
    Utils::Vec<nodeR_t> findNumExact(const node_t& snap1, const N number);
    constexpr auto      findNumExactF = []<Meta::Numeric N>(const N number)
    {
        return [number]<OpCompatible node_t>(const node_t& snap1) mutable
        { return findNumExact(snap1, number); };
    };

    // Inclusive.

    template <OpCompatible node_t, Meta::Numeric N,
              OpCompatible nodeR_t = node_t>
    Utils::Vec<nodeR_t> findNumWithinRange(const node_t& snap1, const N& min,
                                           const N& max);

    // for rvalues
    template <template <typename...> typename OuterContainer,
              template <typename...> typename InnerContainer, typename T>
    auto consolidate(OuterContainer<InnerContainer<T>>&& range)
    {
        InnerContainer<T> result;
        auto              backInserter = std::back_inserter(result);
        for (InnerContainer<T>& inner : range)
        {
            std::move(inner.begin(), inner.end(), backInserter);
        }
        return result;
    }

    // For lvalues
    template <template <typename...> typename OuterContainer,
              template <typename...> typename InnerContainer, typename T>
    auto consolidate(OuterContainer<InnerContainer<T>>& range)
    {
        InnerContainer<T> result;
        auto              backInserter = std::back_inserter(result);
        for (InnerContainer<T>& inner : range)
        {
            std::move(inner.begin(), inner.end(), backInserter);
        }
        return result;
    }

}

// Implementation
namespace rmf
{
    /******************************/
    /* Binary Snapshot Operations */
    /******************************/
    template <OpCompatible node1_t, OpCompatible node2_t, OpCompatible nodeR_t>
    Utils::Vec<nodeR_t> findChanged(const node1_t&   nodeSnap1,
                                    const node2_t&   nodeSnap2,
                                    const uintptr_t& compareSize)
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

    template <OpCompatible node1_t, OpCompatible node2_t, OpCompatible nodeR_t>
    Utils::Vec<nodeR_t> findUnchanged(const node1_t&   nodeSnap1,
                                      const node2_t&   nodeSnap2,
                                      const uintptr_t& compareSize)
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

    template <OpCompatible node1_t, OpCompatible node2_t, Meta::Numeric N,
              OpCompatible nodeR_t>
    Utils::Vec<nodeR_t> findNumChanged(const node1_t& nodeSnap1,
                                       const node2_t& nodeSnap2,
                                       const N&       minDifference)
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
    template <OpCompatible node1_t, OpCompatible node2_t, Meta::Numeric N,
              OpCompatible nodeR_t>
    Utils::Vec<nodeR_t> findNumUnchanged(const node1_t& nodeSnap1,
                                         const node2_t& nodeSnap2,
                                         const N&       maxDifference)
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

    template <OpCompatible node_t, OpCompatible nodeR_t>
    Utils::Vec<nodeR_t> findString(const node_t&          nodeSnap,
                                   const std::string_view str)
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

    template <OpCompatible node_t, Meta::Numeric N, OpCompatible nodeR_t>
    Utils::Vec<nodeR_t> findNumExact(const node_t& nodeSnap, const N number)
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
    template <OpCompatible node_t, Meta::Numeric N, OpCompatible nodeR_t>
    Utils::Vec<nodeR_t> findNumWithinRange(const node_t& nodeSnap, const N& min,
                                           const N& max)
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

    template <typename... Features>
    Utils::Vec<Node<Map, Features...>> getMaps(pid_t pid)
    {
        using NodeBase = Node<Map, Features...>;
        std::ifstream        memoryMapFile(std::format("/proc/{}/maps", pid));
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
