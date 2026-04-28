#ifndef struct_registry_hpp_INCLUDED
#define struct_registry_hpp_INCLUDED
#include "rmf/node.hpp"
#include "rmf/snapshot.hpp"
#include <memory>
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

    // Problemo - How do i determine different methods for the several types
    // of types available? Mixins with reverse inheritance won't work because
    // this needs to be done during runtime.
    // I essentially need to distinguish between the 4 different types -
    // 1. Composite (struct)
    // 2. Pointers (structName*, int*, etc)
    // 3. Ararys (int[10], char[100], etc.)
    // 4. Primitive (ints, floats, etc.)
    // let's try a semi-polymorphic semi-mixins solution.
    // IE Rather than nodes being "typed", we'll also have it inherit
    // from the actual derived classes, such as Structs, Fields etc.
    // Each of these would have their own versions of the
    enum class TypeKind
    {
        Invalid,
        Struct,
        Pointer,
        Primitive,
        Array
    };
    class Typed;

    class TypeBase // base
    {
      private:
        static inline sptr<std::string> DefaultName =
            std::make_shared<std::string>("Undefined name!");

        TypeKind          m_kind      = TypeKind::Invalid;
        size_t            m_alignment = 0;
        size_t            m_size      = 0;
        sptr<std::string> m_name      = DefaultName;

      public:
        constexpr static inline TypeKind selfKind = TypeKind::Invalid;
        TypeKind                         kind();
        size_t                           alignment();
        size_t                           size();
        const std::string_view           name();
        template <typename Self>
        bool isSelf(this const Self& s)
        { return Self::selfKind == s.selfKind; }
    };

    class Struct : public TypeBase
    {
      private:
        // Struct data hidden behind a shared pointer
        struct Data
        {
            using FieldName = std::string;
            std::unordered_map<FieldName, Typed> fields;
        };
        sptr<Data> m_data;

      public:
        constexpr static inline TypeKind selfKind = TypeKind::Struct;
        // Only if it it is a composite type.
        Utils::ErrU<Field> field(const strview fieldName);
        Field              operator[](const strview fieldName);

        std::span<Field>   fields();

        bool               hasField(const strview fieldName);

        size_t             numFields();
    };

    class Pointer : public TypeBase
    {
      public:
        constexpr static inline TypeKind selfKind = TypeKind::Pointer;

        Typed                            pointee();
        // mixin?

        // Self must be a node with a snapshot.
        // Get the value of the snapshot
        // The snapshot also must be valid.
        template <NodeWithFeatures<Snapshot> Self>
        uintptr_t value(this const Self& self);
    };

    class Primitive : public TypeBase
    {
      public:
        constexpr static inline TypeKind selfKind =
            TypeKind::Primitive;

        template <NodeWithFeatures<Snapshot> Self>
        auto value(this const Self& self);
    };

    class Array : public TypeBase
    {
      public:
        constexpr static inline TypeKind selfKind = TypeKind::Array;
        Typed                            memberType();

        // Useful for getting the exact node's map details at a certain ind.
        template <NodeWithFeatures<Map> Self>
        Self nodeAtInd(this const Self& self, size_t ind);

        template <NodeWithFeatures<Map> Self>
        auto valueAtInd(this const Self& self, size_t ind);

        template <NodeWithFeatures<Map, Snapshot> Node_t>
        auto value(const Node_t& node);
    };

    class Typed
        : public std::variant<Primitive, Pointer, Array, Struct>
    {
      public:
        using SelfType =
            std::variant<Primitive, Pointer, Array, Struct>;
        using SelfType::variant;

        // Allow implicit conversions for ease of use
        template <typename T>
        operator T();

        // Convert to Struct
        Struct struct_();

        // Convert to Array
        Array array();

        // Convert to Pointer
        Pointer pointer();

        // Convert to Primitive
        Primitive primitive();
    };

    class Field
    {
      private:
        Typed m_parentType;
        Typed m_innerType;

      public:
        size_t offset();
        size_t size();
        Typed  parentType();
        Typed  innerType();

        // Reshapes to a node with mf::Typed or something similar.
        template <IsNode T>
        T reshapeNode(const T& node);
    };

    class TypeBuilder;

    class TypeRegistry
    {
      private:
        struct Data
        {
            std::unordered_map<std::string, std::string> aliases;
            std::unordered_map<std::string, Struct>      structs;
            std::unordered_map<std::string, Pointer>     pointers;
            std::unordered_map<std::string, Primitive>   primitives;
            std::unordered_map<std::string, Array>       arrays;
        };
        sptr<Data> m_data;

      public:
        TypeBuilder struct_(const strview name);

        // search all. Use implicit conversion.
        Utils::ErrU<Typed> operator[](const strview name);

        // Find a type by name.
        Utils::ErrU<Struct>    getStruct(const strview name);
        Utils::ErrU<Pointer>   getPointer(const strview name);
        Utils::ErrU<Primitive> getPrimitive(const strview name);
        Utils::ErrU<Array>     getArray(const strview name);
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
