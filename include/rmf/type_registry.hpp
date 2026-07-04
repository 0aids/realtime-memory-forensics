#pragma once
#include "rmf/config.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <map>
#include <unordered_map>
#include <vector>

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

namespace rmf
{
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
    enum class PType
    {
#define X(prefix, ...) prefix##__VA_ARGS__,
        RMF_PRIM_TYPES(X)
#undef X
    };

    // Holds a parent type and the actually referred to type.
    class Field;
    class Typed;
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

        std::weak_ptr<StructData>   parentStruct;
        std::weak_ptr<BaseTypeData> targetType;
        ssize_t                     offset = 0;
    };

    // No support for nested structs cause i'm lazy as fuck.
    struct StructData : public BaseTypeData
    {
        // Ordered map. Look up by name.
        // Ordered to allow iteration.
        // FieldData hashes by its name.
        // Fields are owned by structs.
        std::map<std::string, std::shared_ptr<FieldData>> fields;
    };

    struct PointerData : public BaseTypeData
    {
        std::weak_ptr<BaseTypeData> targetType;
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
        std::weak_ptr<BaseTypeData> m_baseData;
        friend class StructBuilder;
        friend class TypeRegistry;
        friend class Struct;
        Typed(std::weak_ptr<BaseTypeData> data);
        static Typed makeFromWptr(std::weak_ptr<BaseTypeData> data);

      public:
        ssize_t                size() const;
        ssize_t                alignment() const;
        Type                   type() const;
        const std::string_view name() const;
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

    class Struct : public Typed
    {
        std::weak_ptr<StructData> m_data;

      public:
        // Not really sure about the constructor situation here.
        Struct(const Typed&);
        Struct()                                      = delete;
        Struct(Struct&&)                              = default;
        Struct(const Struct&)                         = default;
        Struct&              operator=(Struct&&)      = default;
        Struct&              operator=(const Struct&) = default;

        std::optional<Field> getField(const std::string& str);

        // Asserts that it exists - Otherwise throws.
        Field operator[](const std::string_view str);

        struct Iterator
        {
            Iterator(std::weak_ptr<StructData> parent);
            Iterator()                           = delete;
            Iterator(Iterator&&)                 = default;
            Iterator(const Iterator&)            = default;
            Iterator& operator=(Iterator&&)      = default;
            Iterator& operator=(const Iterator&) = default;

            using iterator_category = std::forward_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = const Field;
            using pointer           = const Field; // or also value_type*
            using reference         = const Field; // or also value_type&

            reference operator*() const;
            pointer   operator->();

            // Prefix increment
            Iterator& operator++();

            // Postfix increment
            Iterator    operator++(int);

            friend bool operator==(const Iterator& a, const Iterator& b);
            friend bool operator!=(const Iterator& a, const Iterator& b);

            std::weak_ptr<StructData> m_parent;
            std::map<std::string, std::shared_ptr<FieldData>>::const_iterator
                m_currentIter;
        };

        // Only const interation!
        Iterator                            begin() const;
        Iterator                            end() const;

        std::vector<const std::string_view> getFieldNames() const;
        Field operator[](const std::string_view) const;
        bool  containsField(const std::string_view field);
        bool  containsField(const Field& field);
    };

    // Mutually exclusive with "Typed" nodes.
    class Pointer : public Typed
    {
      protected:
        std::weak_ptr<PointerData> m_pointerData;

      public:
        Pointer(const Typed&);
        Pointer()                          = delete;
        Pointer(Pointer&&)                 = default;
        Pointer(const Pointer&)            = default;
        Pointer& operator=(Pointer&&)      = default;
        Pointer& operator=(const Pointer&) = default;

        Typed    targetType() const;
        // // Returns a new MemoryRegionTyped
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
        template <Numeric T>
        operator T() const;
    };

    class Primitive : public Typed
    {
        std::weak_ptr<PrimitiveData> m_data;

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
      protected:
        std::weak_ptr<ArrayData> m_data;

      public:
        Array(const Typed&);
        Array()                        = delete;
        Array(Array&&)                 = default;
        Array(const Array&)            = default;
        Array& operator=(Array&&)      = default;
        Array& operator=(const Array&) = default;

        Typed  targetType() const;

        // // Returns a new MemoryRegionTyped
    };

    class Field : public Typed
    {
        std::weak_ptr<FieldData> m_data;

      public:
        Field(const Typed& typed);
        Field()                        = delete;
        Field(Field&&)                 = default;
        Field(const Field&)            = default;
        Field& operator=(Field&&)      = default;
        Field& operator=(const Field&) = default;
        Field(std::weak_ptr<FieldData>);

        ssize_t offset() const;

        // Get back the struct from which a field belongs to.

        // // Returns a new MemoryRegionTyped
    };

    class StructBuilder;

    // Consider adding these options later
    // struct StructOptions {
    //     enum {
    //         UNPACKED = 0,
    //         PACKED,
    //     } packed;
    //     ssize_t alignas_;
    // };

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
            std::unordered_map<std::string, std::shared_ptr<StructData>>
                                                        structs{};
            std::vector<std::shared_ptr<PrimitiveData>> primitives{};
            std::unordered_map<std::string, std::shared_ptr<ArrayData>> arrs{};
            std::unordered_map<std::string, std::shared_ptr<PointerData>>
                ptrs{};
        };
        std::shared_ptr<Data> m_data = std::make_shared<Data>();
        // Have to use a factory because of the stupid ass
        // deleted default constructors of primitives.
        TypeRegistry(std::vector<std::shared_ptr<PrimitiveData>>&& primitives,
                     PrimitiveList&& primitiveList);

      public:
        static TypeRegistry New();
        TypeRegistry()                               = delete;
        TypeRegistry(const TypeRegistry&)            = default;
        TypeRegistry(TypeRegistry&&)                 = default;
        TypeRegistry& operator=(const TypeRegistry&) = default;
        TypeRegistry& operator=(TypeRegistry&&)      = default;

        StructBuilder defStruct(const std::string_view name);
        Pointer       ptrTo(Typed T);
        Array         arrOf(Typed T, ssize_t size);
        // Throws an error if it doesn't exist, via invalid key error or whatever
        // that std::unordered_map throws.
        Struct struct_(const std::string_view name);

        // Get a struct.
        Struct operator[](const std::string_view name);

        // Primitives defined here
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
        TypeRegistry                m_parent;
        std::string                 m_name;
        std::shared_ptr<StructData> m_structData;
        friend class TypeRegistry;
        StructBuilder(const TypeRegistry& parent, const std::string_view name);

      public:
        StructBuilder()                                 = delete;
        StructBuilder(const StructBuilder&)             = delete;
        StructBuilder(StructBuilder&&)                  = default;
        StructBuilder&  operator=(const StructBuilder&) = delete;
        StructBuilder&  operator=(StructBuilder&&)      = default;

        StructBuilder&& field(Typed type, const std::string_view name);
        Struct          end();
    };
} // namespace rmf

// Specialisations for hashing fields.
template <>
struct std::hash<rmf::BaseTypeData>
{
    using Hashee = rmf::BaseTypeData;
    std::size_t operator()(const Hashee& h)
    {
        return std::hash<std::string>{}(h.name);
    }
};
template <>
struct std::hash<std::shared_ptr<rmf::BaseTypeData>>
{
    using Hashee = std::shared_ptr<rmf::BaseTypeData>;
    std::size_t operator()(const Hashee& h)
    {
        return std::hash<std::string>{}(h->name);
    }
};

template <>
struct std::hash<std::weak_ptr<rmf::BaseTypeData>>
{
    using Hashee = std::weak_ptr<rmf::BaseTypeData>;
    std::size_t operator()(const Hashee& h)
    {
        return std::hash<std::string>{}(h.lock()->name);
    }
};

// TODO: Make it satisfy random access iterator.
static_assert(std::input_iterator<rmf::Struct::Iterator>,
              "Struct Iterator should match iterator specs");

#ifndef RMF_NO_CLEANUP_MACROS
#undef RMF_PRIM_TYPES
#undef RMF_PTR_SIZE
#endif
