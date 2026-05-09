#ifndef struct_registry_hpp_INCLUDED
#define struct_registry_hpp_INCLUDED
#include "rmf/node.hpp"
#include "rmf/snapshot.hpp"
#include "rmf/utils/other.hpp"
#include "rmf/utils/vec.hpp"
#include <memory>
#include <ranges>
#include <map>
#include <unordered_map>
#include <string_view>
#include <rmf/utils/expect.hpp>
#include <unordered_set>
#include <variant>
// Designing using funky inheritance and pointer spamming.
// Weak and shared pointer spam because is a Directed, possibly cyclic graph.
// Shared pointers are the majority owners. Unique pointers doesnt allow a clean
// interfacing for holding non-owning views. wptrs are for traversal.

// Structure:
// Data holders - This are held as proper polymporphic types, but no
//   actual member methods.
// Viewers - Are polymorphic (sort of?), more like extensions.
// Do not own, are used as mixins with nodes.

// Macro shit to generate the standard sizes.
#define RMF_PTR_SIZE sizeof(void*)

#define RMF_PRIM_TYPES(X)                                                      \
    X(v, 0)                                                                    \
    X(u, 8)                                                                    \
    X(u, 16)                                                                   \
    X(u, 32)                                                                   \
    X(u, 64)                                                                   \
    X(i, 8)                                                                    \
    X(i, 16)                                                                   \
    X(i, 32)                                                                   \
    X(i, 64)                                                                   \
    X(f, 32)                                                                   \
    X(f, 64)

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

// Primitive types
#define X(prefix, ...) prefix##__VA_ARGS__,
    enum class PType
    {
        RMF_PRIM_TYPES(X)
    };
#undef X

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
        // We do not own our parent or the target!!

        wptr<StructData>   parentStruct;
        wptr<BaseTypeData> targetType;
        ssize_t            offset = 0;
    };

    // No support for nested structs cause i'm lazy as fuck.
    struct StructData : public BaseTypeData
    {
        // Ordered map. Look up by name.
        // Ordered to allow iteration.
        // FieldData hashes by its name.
        // Fields are owned by structs.
        std::map<std::string, sptr<FieldData>> fields;
    };

    struct PointerData : public BaseTypeData
    {
        wptr<BaseTypeData> targetType;
    };

    struct PrimitiveData : public BaseTypeData
    {
        PType primativeType = PType::v0;
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
      protected:
        wptr<BaseTypeData> m_baseData;
        friend class StructBuilder;
        friend class TypeRegistry;
        Typed(wptr<BaseTypeData> data);
        static Typed makeFromWptr(wptr<BaseTypeData> data);

      public:
        ssize_t       size() const;
        ssize_t       alignment() const;
        Type          type() const;
        const strview name() const;
        Typed()                        = delete;
        Typed(Typed&&)                 = default;
        Typed(const Typed&)            = default;
        Typed& operator=(Typed&&)      = default;
        Typed& operator=(const Typed&) = default;

        // Explicit conversions? as these are checked.
        // explicit operator Pointer();
        // explicit operator Array();
        // explicit operator Struct();
        // explicit operator Primitive();
    };

    template <typename T>
    concept FieldDeducible =
        requires { std::same_as<T, strview> || std::same_as<T, Field>; };

    template <typename T>
    concept TypeDeducible = requires {
        std::same_as<T, PType> || std::same_as<T, strview> ||
            std::same_as<T, Struct>;
    };

    template <typename T>
    concept StructDeducible =
        requires { std::same_as<T, strview> || std::same_as<T, Struct>; };

    class Struct : public Typed
    {
        wptr<StructData> m_data;

      public:
        // Not really sure about the constructor situation here.
        Struct(const Typed&);
        Struct()                                    = delete;
        Struct(Struct&&)                            = default;
        Struct(const Struct&)                       = default;
        Struct&            operator=(Struct&&)      = default;
        Struct&            operator=(const Struct&) = default;

        Utils::ErrU<Field> getField(const std::string& str);

        // Asserts that it exists - Otherwise throws.
        Field operator[](const strview str);

        // Consider adding functor for mapped operations?
        // Creates a typed version of a node
        template <IsNode T, IsNode ResultNode = T::template AddFeature<Typed>>
        // Strange error saying that i'm using a deleted constructor
        // when i haven't even defined the function?
        ResultNode nodify(const T& node);

        // Creates a typed version of a node, from a specified field.
        template <IsNode T, FieldDeducible ForS,
                  IsNode ResultNode = T::template AddFeature<Typed>>
        ResultNode nodifyFromField(const T& node, const ForS& field);

        // -- Relevant mixin operations --

        // Get the node at a specific field.
        // This node is technically a "SubNode", but we make no distinction.
        template <IsNode Node, FieldDeducible ForS,
                  IsNode ResultNode = Node::template SwapFeature<Struct, Typed>>
        ResultNode fieldNode(this const Node&, const ForS& field);

        // Gets the actual buffer at a specific field, as either as a desired
        // range.
        template <NodeWithFeatures<Snapshot> Node>
        SnapshotBuffer bytesAtField(this const Node&, const Field& field);
    };

    // Mixinable
    // Mutually exclusive with "Typed" nodes.
    class Pointer : public Typed
    {
      protected:
        wptr<PointerData> m_data;

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
            requires NodeWithFeatures<std::ranges::range_value_t<MapRange>, Map>
        Node::template SwapFeature<Pointer, Typed>
        targetNode(this const Node& node, const MapRange& maps);
    };

    // A temporary holder of data of unknown type. Used by primitive
    // because it cannot convert string data into a defined type during runtime.
    // Is an ephemeral type of size 8 bytes to fit all standard
    // primitives. Implicitly casts to any standard number (including floats).
    struct Unknown
    {
        uint64_t value = 0;

        template <typename T>
        T as();

        // Can implicitly cast to any integer or float.
        template <Utils::Meta::Numeric T>
        operator T() const;
    };

    class Primitive : public Typed
    {
        wptr<PrimitiveData> m_data;

      public:
        Primitive(const Typed&);
        Primitive()                            = delete;
        Primitive(Primitive&&)                 = default;
        Primitive(const Primitive&)            = default;
        Primitive& operator=(Primitive&&)      = default;
        Primitive& operator=(const Primitive&) = default;

        template <NodeWithFeatures<Snapshot> Node>
        Unknown value(this const Node& node);
    };

    class Array : public Typed
    {
      protected:
        wptr<ArrayData> m_data;

      public:
        Array(const Typed&);
        Array()                        = delete;
        Array(Array&&)                 = default;
        Array(const Array&)            = default;
        Array& operator=(Array&&)      = default;
        Array& operator=(const Array&) = default;

        Typed  targetType() const;

        // Create a node at the relevant index of the array,
        // with included bounds checking which throws if you are stupid.
        template <IsNode Node>
        Node::template SwapOrAddFeature<Array, Typed> nodeAt(ssize_t i) const;
    };

    class Field : public Typed
    {
        wptr<FieldData> m_data;

      public:
        Field(const Typed& typed);
        Field()                        = delete;
        Field(Field&&)                 = default;
        Field(const Field&)            = default;
        Field& operator=(Field&&)      = default;
        Field& operator=(const Field&) = default;
        Field(wptr<FieldData>);
        template <IsNode Node>
        Node::template AddFeature<Typed> nodify(const Node& node);
    };

    class StructBuilder;

    // Consider refactoring this to be an implementation,
    // and use a static constexpr to have the classic
    // constructors without the stupid ahh factory method.
    // The monolithic class keeping track and owning all
    // types.
    class TypeRegistry
    {
      public:
        struct PrimitiveList;

      private:
        struct Data
        {
            std::unordered_map<std::string, sptr<StructData>>  structs{};
            std::vector<sptr<PrimitiveData>>                   primitives{};
            std::unordered_map<std::string, sptr<ArrayData>>   arrs{};
            std::unordered_map<std::string, sptr<PointerData>> ptrs{};
        };
        sptr<Data> m_data = std::make_shared<Data>();
        // Have to use a factory because of the stupid ass
        // deleted default constructors of primitives.
        TypeRegistry(std::vector<sptr<PrimitiveData>>&& primitives,
                     PrimitiveList&&                    primitiveList);

      public:
        static TypeRegistry Make();
        TypeRegistry()                               = delete;
        TypeRegistry(const TypeRegistry&)            = default;
        TypeRegistry(TypeRegistry&&)                 = default;
        TypeRegistry& operator=(const TypeRegistry&) = default;
        TypeRegistry& operator=(TypeRegistry&&)      = default;

        StructBuilder defStruct(const strview name);
        Pointer       ptrTo(Typed T);
        Array         arrOf(Typed T, ssize_t size);
        // Throws an error if it doesn't exist, via invalid key error or whatever
        // that std::unordered_map throws.
        Struct struct_(const strview name);

        // Get a struct.
        Utils::ErrU<Struct> operator[](const strview name);
        // Primitives defined here?
        // During initial construction primitive data are added to data,
        // and then these will be constructed properly.
        struct PrimitiveList
        {
#define X(name, size) Primitive name##size;
            RMF_PRIM_TYPES(X)
#undef X
        } prim;
        friend class StructBuilder;
    };

    class StructBuilder
    {
      private:
        TypeRegistry     m_parent;
        std::string      m_name;
        sptr<StructData> m_structData;
        friend class TypeRegistry;
        StructBuilder(const TypeRegistry& parent, const strview name);

      public:
        StructBuilder()                                 = delete;
        StructBuilder(const StructBuilder&)             = delete;
        StructBuilder(StructBuilder&&)                  = default;
        StructBuilder&  operator=(const StructBuilder&) = delete;
        StructBuilder&  operator=(StructBuilder&&)      = default;

        StructBuilder&& field(Typed type, const strview name);
        Struct          end();
    };
};

// Specialisations for hashing fields.
template <>
struct std::hash<RealtimeMemoryForensics::BaseTypeData>
{
    using Hashee = RealtimeMemoryForensics::BaseTypeData;
    std::size_t operator()(const Hashee& h)
    {
        return std::hash<std::string>{}(h.name);
    }
};
template <>
struct std::hash<std::shared_ptr<RealtimeMemoryForensics::BaseTypeData>>
{
    using Hashee = std::shared_ptr<RealtimeMemoryForensics::BaseTypeData>;
    std::size_t operator()(const Hashee& h)
    {
        return std::hash<std::string>{}(h->name);
    }
};

template <>
struct std::hash<std::weak_ptr<RealtimeMemoryForensics::BaseTypeData>>
{
    using Hashee = std::weak_ptr<RealtimeMemoryForensics::BaseTypeData>;
    std::size_t operator()(const Hashee& h)
    {
        return std::hash<std::string>{}(h.lock()->name);
    }
};

namespace RealtimeMemoryForensics
{
    template <IsNode T, IsNode ResultNode>
    // Strange error saying that i'm using a deleted constructor
    // when i haven't even defined the function?
    ResultNode Struct::nodify(const T& node)
    {
        rmf_TODO();
    }
    template <IsNode Node>
    Node::template AddFeature<Typed> Field::nodify(const Node& node)
    {
        return node.addFeature(*this);
    }
}

#ifndef RMF_NO_CLEANUP_MACROS
#undef RMF_PRIM_TYPES
#undef RMF_PTR_SIZE
#endif

#endif // struct_registry_hpp_INCLUDED
