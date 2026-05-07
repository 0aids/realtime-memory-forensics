#ifndef struct_registry_hpp_INCLUDED
#define struct_registry_hpp_INCLUDED
#include "rmf/node.hpp"
#include "rmf/snapshot.hpp"
#include "rmf/utils/vec.hpp"
#include <memory>
#include <ranges>
#include <set>
#include <string_view>
#include <rmf/utils/expect.hpp>
#include <unordered_set>
#include <variant>
// Designing using funky inheritance and pointer spamming.
// Weak and shared pointer spam because is a Directed, possibly cyclic graph.
// Shared pointers are the majority owners. Unique pointers doesnt allow a clean
// interfacing for holding non-owning views. wptrs are for traversal.
namespace RealtimeMemoryForensics
{
    template <typename T>
    using sptr = std::shared_ptr<T>;
    template <typename T>
    using wptr    = std::weak_ptr<T>;
    using strview = std::string_view;

    // Holds a parent type and the actually referred to type.
    class Field;
    class Typed;

    enum class Type
    {
        Undefined = 0,
        Struct,
        Array,
        Pointer,
        Primitive,
        Field,
    };

    // Base type data
    // All impls hold only data.
    // and this base type. This means we can always
    // at least check a type's name, size and alignment
    // without casting all the time.
    // These are normally not used by themselves, and are held
    // by shared pointers (so we can make use of virtuality?)
    struct BaseTypeData
    {
        ssize_t size = 0;
        // Alignment of a type.
        ssize_t alignment = 0;
        // Name of the type, unless a field.
        std::string name = "Undefined!";
        Type        type = Type::Undefined;
    };

    struct StructData;

    struct FieldData : public BaseTypeData
    {
        // !! The name of the field is not actually the type name.

        wptr<StructData>   parentStruct;
        wptr<BaseTypeData> targetType;
        ssize_t            offset = 0;
    };

    // No support for nested structs cause i'm lazy as fuck.
    struct StructData : public BaseTypeData
    {
        // Ordered set. Look up by name.
        // Ordered to allow iteration.
        // FieldData hashes by its name.
        // Fields are owned by structs.
        // Shared pointer is to allow weak pointer viewing.
        std::set<sptr<FieldData>> fields;
    };

    struct PointerData : public BaseTypeData
    {
        wptr<BaseTypeData> targetType;
    };

    struct PrimitiveData : public BaseTypeData
    {
        // Nothing, primitives have enough data.
    };

    struct ArrayData : public BaseTypeData
    {
        ssize_t arrayCount = 0;
    };

    class Pointer;
    class Array;
    class Struct;
    class Primitive;

    // Type erased data accessor.
    // Has explicit conversions to its children.
    class Typed
    {
        sptr<BaseTypeData> m_data;

      public:
        ssize_t       size() const;
        ssize_t       alignment() const;
        Type          type() const;
        const strview name() const;

        // Explicit conversions? as these are checked.
        // explicit operator Pointer();
        // explicit operator Array();
        // explicit operator Struct();
        // explicit operator Primitive();
    };

    template <typename T>
    concept FieldOrStrview = requires {
        std::same_as<T, strview> || std::same_as<T, Field>;
    };

    class Struct : public Typed
    {
        sptr<StructData> m_data;

      public:
        // Not really sure about the constructor situation here.
        Struct(const Typed&);
        Struct()                                    = default;
        Struct(Struct&&)                            = default;
        Struct(const Struct&)                       = default;
        Struct&            operator=(Struct&&)      = default;
        Struct&            operator=(const Struct&) = default;

        Utils::ErrU<Field> getField(const strview str);

        // Consider adding functor for mapped operations?
        // Creates a typed version of a node
        template <IsNode T,
                  IsNode ResultNode = T::template AddFeature<Typed>>
        ResultNode nodify(const T& node);

        // Creates a typed version of a node, from a specified field.
        template <IsNode T, FieldOrStrview ForS,
                  IsNode ResultNode = T::template AddFeature<Typed>>
        ResultNode nodifyFromField(const T& node, const ForS& field);

        // -- Relevant mixin operations --

        // Get the node at a specific field.
        // This node is technically a "SubNode", but we make no distinction.
        template <IsNode Node, FieldOrStrview ForS,
                  IsNode ResultNode =
                      Node::template SwapFeature<Struct, Typed>>
        ResultNode fieldNode(this const Node&, const ForS& field);

        // Gets the actual buffer at a specific field, as either as a desired
        // range.
        template <NodeWithFeatures<Snapshot> Node>
        SnapshotBuffer bytesAtField(this const Node&,
                                    const Field& field);
    };

    // Mixinable
    // Mutually exclusive with "Typed" nodes.
    class Pointer : public Typed
    {
        sptr<PointerData> m_data;

      public:
        Pointer(const Typed&);
        Pointer()                          = delete;
        Pointer(Pointer&&)                 = default;
        Pointer(const Pointer&)            = default;
        Pointer& operator=(Pointer&&)      = default;
        Pointer& operator=(const Pointer&) = default;

        Typed    targetType() const;

        // Get the value of the pointer which this object is referencing.
        template <NodeWithFeatures<Snapshot> Node>
        uintptr_t value(this const Node& node);

        // Get the node at the pointer's target. Dereferences it's internal
        // snapshot to find the value, then creates a relevant node using that value.
        // Requires maps to query the parent region. If it cannot find the parent region,
        // it returns the region without a specified parent region.
        template <NodeWithFeatures<Snapshot> Node, typename MapRange>
            requires NodeWithFeatures<
                std::ranges::range_value_t<MapRange>, Map>
        Node::template SwapFeature<Pointer, Typed>
        targetNode(this const Node& node, const MapRange& maps);
    };

    // A temporary holder of data of unknown type. Used by primitive
    // because it cannot convert string data into a defined type during runtime.
    // Is an ephemeral type of size 8 bytes to fit all standard
    // sizes.
    struct Unknown
    {
        uint64_t value = 0;

        template <typename T>
        T as();

        // Can implicitly cast to any integer or float.
        template <Utils::Meta::Numeric T>
        operator T();
    };

    class Primitive : public Typed
    {
        sptr<PrimitiveData> m_data;

      public:
        Primitive(const Typed&);
        Primitive()                            = delete;
        Primitive(Primitive&&)                 = default;
        Primitive(const Primitive&)            = default;
        Primitive& operator=(Primitive&&)      = default;
        Primitive& operator=(const Primitive&) = default;
    };

    class Array : public Typed
    {
        sptr<ArrayData> m_data;

      public:
        Array(const Typed&);
        Array()                        = delete;
        Array(Array&&)                 = default;
        Array(const Array&)            = default;
        Array& operator=(Array&&)      = default;
        Array& operator=(const Array&) = default;

        // Create a node at the relevant index of the array,
        // with included bounds checking which throws if you are stupid.
        template <IsNode Node>
        Node::template SwapFeature<Array, Typed>
        nodeAt(ssize_t i) const;
    };

    class Field : public Typed
    {
        sptr<FieldData> m_data;

      public:
        template <IsNode Node>
        Node::template SwapFeature<Field, Struct>
        nodifyFromField(this const Node& node);
    };

    class TypeBuilder;

    // The monolithic class keeping track and owning all
    // types.
    class TypeRegistry
    {
      private:
        struct Data
        {
            std::unordered_set<sptr<StructData>>    structs;
            std::unordered_set<sptr<PrimitiveData>> primatives;
            std::unordered_set<sptr<ArrayData>>     arrs;
            std::unordered_set<sptr<PointerData>>   ptrs;
        };
        sptr<Data> m_data;

      public:
        TypeBuilder defStruct(const strview name);

        // Get a struct.
        Utils::ErrU<Struct> operator[](const strview name);
    };

    class TypeBuilder
    {
      private:
        TypeRegistry m_parent;
        friend class TypeRegistry;
        TypeBuilder(TypeRegistry& parent);

      public:
        TypeBuilder()                               = delete;
        TypeBuilder(const TypeBuilder&)             = delete;
        TypeBuilder(TypeBuilder&&)                  = default;
        TypeBuilder&  operator=(const TypeBuilder&) = delete;
        TypeBuilder&  operator=(TypeBuilder&&)      = default;

        TypeBuilder&& primitive(const strview type,
                                const strview name);
        TypeBuilder&& pointer(const strview type, const strview name);
        TypeBuilder&& array(const strview type, const strview name);
        TypeBuilder&& array(const strview type, ssize_t size,
                            const strview name);
        TypeBuilder&& struct_(const strview type, const strview name);
        Struct        end();
    };
};

// Specialisations for hashing fields.
template <>
struct std::hash<RealtimeMemoryForensics::BaseTypeData>
{
    using Hashee = RealtimeMemoryForensics::BaseTypeData;
    std::size_t operator()(const Hashee& h)
    { return std::hash<std::string>{}(h.name); }
};
template <>
struct std::hash<
    std::shared_ptr<RealtimeMemoryForensics::BaseTypeData>>
{
    using Hashee =
        std::shared_ptr<RealtimeMemoryForensics::BaseTypeData>;
    std::size_t operator()(const Hashee& h)
    { return std::hash<std::string>{}(h->name); }
};
template <>
struct std::hash<std::weak_ptr<RealtimeMemoryForensics::BaseTypeData>>
{
    using Hashee =
        std::weak_ptr<RealtimeMemoryForensics::BaseTypeData>;
    std::size_t operator()(const Hashee& h)
    { return std::hash<std::string>{}(h.lock()->name); }
};
#endif // struct_registry_hpp_INCLUDED
