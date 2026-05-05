#ifndef struct_registry_hpp_INCLUDED
#define struct_registry_hpp_INCLUDED
#include "rmf/node.hpp"
#include "rmf/snapshot.hpp"
#include "rmf/utils/vec.hpp"
#include <memory>
#include <ranges>
#include <string_view>
#include <rmf/utils/expect.hpp>
#include <unordered_map>
#include <variant>
namespace RealtimeMemoryForensics
{
    // Will design with  inheritance.
    // That's the only way I can think of doing this in an efficient manner.
    // Mixins - Not suitable because need runtime
    // Variant - I want methods.
    // No other solutions I can think of.
    template <typename T>
    using sptr    = std::shared_ptr<T>;
    using strview = std::string_view;

    // Holds a parent type and the actually referred to type.
    class Field;
    class Typed;

    // Who has the mixin Methods?
    // Typed holds a variant. If we cast downwards then we
    // still have the same data.
    // So a standalone struct can still work.
    class Struct
    {
      private:
        struct Data
        {
        };
        sptr<Data> m_struct = std::make_shared<Data>();

      public:
        Utils::ErrU<Field> getField(const strview str);

        // If conversions are too slow, add overloads for rvalue references
        // that would move all the data?

        // Creates a typed version of a node
        template <IsNode T,
                  IsNode ResultNode = T::template AddFeature<Typed>>
        ResultNode nodify(T);

        // Turns a range of nodes into a typed version.
        template <std::ranges::range NodeRange,
                  IsNode Node = std::ranges::range_value_t<NodeRange>,
                  IsNode ResultNode = Utils::Vec<
                      typename Node::template AddFeature<Typed>>>
        ResultNode nodify(const NodeRange&);

        // Creates a typed version of a node, from a field.
        // Failed coersions are not added.
        template <IsNode T,
                  IsNode ResultNode = T::template AddFeature<Typed>>
        ResultNode nodifyFromField(T);

        // Turns a range of nodes into a typed version.
        template <std::ranges::range NodeRange,
                  IsNode Node = std::ranges::range_value_t<NodeRange>>
        Utils::Vec<typename Node::template AddFeature<Typed>>
        nodifyFromField(const NodeRange&, const Field& field);

        // -- Relevant mixin operations --

        // Get the node at a specific field.
        template <IsNode Node,
                  IsNode ResultNode =
                      Node::template SwapFeature<Struct, Typed>>
        ResultNode nodeAtField(this const Node&, const Field& field);

        // Gets the actual buffer at a specific field, as either as a desired
        // range
        template <NodeWithFeatures<Snapshot> Node>
        SnapshotBuffer bytesAtField(this const Node&,
                                    const Field& field);
    };

    class Pointer
    {
      private:
        struct Data
        {
        };
        sptr<Data> m_data = std::make_shared<Data>();
    };

    class Primitive
    {
      private:
        struct Data
        {
        };
        sptr<Data> m_data = std::make_shared<Data>();
    };

    class Array
    {
      private:
        struct Data
        {
        };
        sptr<Data> m_data = std::make_shared<Data>();
    };

    // Screw inheritance and pointers. Variant is best.
    class Typed
    {
      private:
        std::variant<Struct, Pointer, Primitive, Array> m_data;

      public:
        // Moves self into the desired type, to access the mixin's data.
        template <IsNode Self, typename Variant>
        Self::template SwapFeature<Typed, Variant>
        typedAs(this Self&& self);
    };

    class Field
    {
        Struct m_source;
        Typed  m_target;

      public:
        // Creates a typed version of a node
        template <IsNode T>
        T::template AddFeature<Typed> nodify(T);

        // Turns a range of nodes into a typed version.
        template <std::ranges::range NodeRange,
                  IsNode Node = std::ranges::range_value_t<NodeRange>>
        Utils::Vec<typename Node::template AddFeature<Typed>>
        nodify(const NodeRange&);
    };

    class TypeBuilder;

    class TypeRegistry
    {
      private:
        struct Data
        {
            std::unordered_map<std::string, Typed> structs;
        };
        sptr<Data> m_data;

      public:
        TypeBuilder        addStruct(const strview name);

        Utils::ErrU<Typed> operator[](const strview name);
    };

    class TypeBuilder
    {
      private:
        TypeRegistry& parent;

      public:
        TypeBuilder&& field(const strview type, const strview name);
        Struct        end();
    };
};

#endif // struct_registry_hpp_INCLUDED
