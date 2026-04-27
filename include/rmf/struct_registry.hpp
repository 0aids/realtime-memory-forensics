#ifndef struct_registry_hpp_INCLUDED
#define struct_registry_hpp_INCLUDED
#include "rmf/node.hpp"
#include <memory>
#include <string_view>
#include <rmf/utils/expect.hpp>
#include <unordered_map>
namespace RealtimeMemoryForensics
{
    // Will design with virtual inheritance.
    // That's the only way I can think of doing this in an efficient manner.
    // Mixins - Not suitable because need runtime
    // Variant - I want methods.
    // No other solutions I can think of.
    template <typename T>
    using sptr    = std::shared_ptr<T>;
    using strview = std::string_view;

    struct TypeData
    {
        // Some stuff here.
        // Maybe variants between pure types, structs and fields
        // Maybe some enum type here for rough information?
        std::string name;
        size_t      size;
        size_t      alignemnt;
        // Variant here?
    };

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

    class Type // base
    {
      public:
        virtual size_t alignment();
        virtual size_t size();
    };

    class Struct : public Type
    {
      public:
        size_t alignment() override;
        size_t size() override;
        // Only if it it is a composite type.
        Utils::ErrU<Field> field(const strview fieldName);
        std::span<Field>   fields();
        size_t             numFields();
    };

    class Pointer : public Type
    {
      public:
        size_t alignment() override;
        size_t size() override;
        Type   pointee();
    };

    class Primitive : public Type
    {
      public:
        size_t alignment() override;
        size_t size() override;
    };

    class Array : public Type
    {
      public:
        size_t alignment() override;
        size_t size() override;
        Type   memberType();
    };

    class Field
    {
      private:
        Type parentType;
        Type innerType;

      public:
        size_t offset();
        size_t size();
        Type   getParent();
        Type   getInner();
        template <typename T>
            requires IsNode<T>
        T reshapeNode(const T& node);
    };

    class TypeBuilder;

    class StructRegistry
    {
      private:
        struct Data
        {
            // ...
        };
        sptr<Data> m_data;

      public:
        TypeBuilder struct_(const strview name);

        // Find a type by name.
        Type operator[](const strview name);
    };

    class TypeBuilder
    {
        StructRegistry parent;

      public:
        TypeBuilder&& field(const strview type, const strview name);
        Type          end(const strview type, const strview name);
    };
};

#endif // struct_registry_hpp_INCLUDED
