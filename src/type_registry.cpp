#include <array>
#include <cassert>
#include <memory>
#include <optional>
#include <print>
#include <stdexcept>
#include <utility>
#include <format>

#define RMF_NO_CLEANUP_MACROS
#include "rmf/type_registry.hpp"
#undef RMF_NO_CLEANUP_MACROS

#include <magic_enum/magic_enum.hpp>
namespace rmf
{

    constexpr std::array<ssize_t, 11> primSizes = {
#define X(name, size) size / 8,
        RMF_PRIM_TYPES(X)
#undef X
    };

    constexpr std::array<const char*, 11> primStr = {
#define X(name, size) #name #size,
        RMF_PRIM_TYPES(X)
#undef X
    };

    constexpr std::array<PType, 11> primTypes = {
#define X(name, size) PType::name##size,
        RMF_PRIM_TYPES(X)
#undef X
    };

    // In order to preconstruct all the stuff with deleted default constructors.
    TypeRegistry TypeRegistry::New()
    {
        std::vector<std::shared_ptr<PrimitiveData>> primitives{};
        // Initialise all the primitive types.
        for (size_t i = 0; i < primTypes.size(); i++)
        {
            primitives.emplace_back(std::make_shared<PrimitiveData>(
                BaseTypeData{
                    .size      = primSizes[i],
                    .alignment = primSizes[i],
                    .name      = primStr[i],
                    .type      = Type::Primitive,
                },
                primTypes[i]));
        }
        // Define the all types.
        PrimitiveList primList = {
#define X(name, size)                                                          \
    .name##size =                                                              \
        Primitive(Typed(primitives[static_cast<size_t>(PType::name##size)])),
            RMF_PRIM_TYPES(X)
#undef X
        };
        return TypeRegistry(std::move(primitives), std::move(primList));
    }

    TypeRegistry::TypeRegistry(
        std::vector<std::shared_ptr<PrimitiveData>>&& primitives,
        PrimitiveList&& primitiveList) : prim(primitiveList)
    {
        m_data->primitives = std::move(primitives);
    }

    StructBuilder TypeRegistry::defStruct(const std::string_view name)
    {
        return StructBuilder(*this, name);
    }

    Pointer TypeRegistry::ptrTo(Typed T)
    {
        // Add ourselves to the map with the name "{struct name}*"
        auto ptr = std::make_shared<PointerData>(PointerData{
            BaseTypeData{
                         .size      = RMF_PTR_SIZE,
                         .alignment = RMF_PTR_SIZE,
                         .name      = std::format("{}*", T.name()),
                         .type      = Type::Pointer,
                         },
            T.m_baseData,
        });
        assert((bool)ptr);

        m_data->ptrs.emplace(ptr->name, ptr);
        return Pointer(Typed(ptr));
    }

    Array TypeRegistry::arrOf(Typed T, ssize_t length)
    {
        auto ptr = std::make_shared<ArrayData>(ArrayData{
            BaseTypeData{
                         .size      = T.size() * length,
                         .alignment = T.alignment(),
                         .name      = std::format("{}[{}]", T.name(), length),
                         .type      = Type::Array,
                         },
            /*arrayCount=*/
            length
        });
        m_data->arrs.emplace(ptr->name, ptr);
        return Array(Typed(ptr));
    }

    Struct TypeRegistry::struct_(const std::string_view name)
    {
        // I forgot how stupid is this. I have to promote it to a fucknig string.
        return Struct(Typed(m_data->structs.at(std::string(name))));
    }

    StructBuilder::StructBuilder(const TypeRegistry&    parent,
                                 const std::string_view name) :
        m_parent(parent), m_name(std::string(name))
    {
        // Create an uninitialised version of ourself.
        m_structData = std::make_shared<StructData>(StructData{
            BaseTypeData{
                         .size      = 0,
                         .alignment = 0,
                         .name      = m_name,
                         .type      = Type::Struct,
                         },
            /*.fields =*/
            {}
        });
        // Add data to the parent as an uninitialised type.
        // So we can refer to ourselves as a pointer.
        m_parent.m_data->structs.emplace(m_name, m_structData);
    }

    StructBuilder&& StructBuilder::field(Typed                  type,
                                         const std::string_view name)
    {
        // Alignment and size calculations are done later, during "end"
        auto field = std::make_shared<FieldData>(FieldData{
            BaseTypeData{
                         .size      = 0,
                         .alignment = 0,
                         .name      = std::string(name),
                         .type      = Type::Field,
                         },
            m_structData,
            type.m_baseData.lock(),
        });
        m_structData->fields.emplace(std::string(name), field);
        return std::move(*this);
    }

    static inline ssize_t calculateOffset(ssize_t currentOffset,
                                          ssize_t memberAlignment)
    {
        if (currentOffset == 0)
            return 0;
        ssize_t padding = (memberAlignment - currentOffset % memberAlignment);
        padding         = (padding == memberAlignment) ? 0 : padding;
        return currentOffset + padding;
    }

    Struct StructBuilder::end()
    {
        // Business logic for setting up everything?
        // Check that we are not referring to ourselves.
        // Alignment, size, and cumulative offset calculation here.
        ssize_t cumulativeOffset = 0;
        ssize_t finalAlignment   = 0;
        for (auto& [name, field] : m_structData->fields)
        {
            const auto& target = field->targetType.lock();
            finalAlignment     = std::max(finalAlignment, target->alignment);
            field->offset =
                calculateOffset(cumulativeOffset, target->alignment);
            field->size = target->size;
            cumulativeOffset += target->size;
            std::println("{}:{} - offset: {}, size: {}, cumulative: {}", name,
                         target->name, field->offset, field->size,
                         cumulativeOffset);
        }
        m_structData->size = calculateOffset(cumulativeOffset, finalAlignment);
        m_structData->alignment = finalAlignment;
        std::println("final size: {}, final alignment: {}", m_structData->size,
                     m_structData->alignment);
        return Struct(Typed(m_structData));
    }

    Typed::Typed(std::weak_ptr<BaseTypeData> data) : m_baseData(data)
    {
    }

    Typed Typed::makeFromWptr(std::weak_ptr<BaseTypeData> data)
    {
        return Typed(data);
    }

    ssize_t Typed::size() const
    {
        return m_baseData.lock()->size;
    }

    ssize_t Typed::alignment() const
    {
        return m_baseData.lock()->alignment;
    }

    Type Typed::type() const
    {
        return m_baseData.lock()->type;
    }

    const std::string_view Typed::name() const
    {
        return m_baseData.lock()->name;
    }

    Struct::Struct(const Typed& typed) : Typed(typed)
    {
        m_data = std::static_pointer_cast<StructData>(m_baseData.lock());
    }

    std::optional<Field> Struct::getField(const std::string& str)
    {
        auto data = m_data.lock();
        if (data->fields.contains(str))
        {
            return Field(Typed::makeFromWptr(data->fields[str]));
        }
        return std::nullopt;
    }

    Pointer::Pointer(const Typed& typed) : Typed(typed)
    {
        m_pointerData =
            std::static_pointer_cast<PointerData>(m_baseData.lock());
    }

    Primitive::Primitive(const Typed& typed) : Typed(typed)
    {
        m_data = std::static_pointer_cast<PrimitiveData>(m_baseData.lock());
    }

    Array::Array(const Typed& typed) : Typed(typed)
    {
        m_data = std::static_pointer_cast<ArrayData>(m_baseData.lock());
    }

    Field::Field(const Typed& typed) : Typed(typed)
    {
        m_data = std::static_pointer_cast<FieldData>(m_baseData.lock());
    }
    ssize_t Field::offset() const
    {
        return m_data.lock()->offset;
    }

    Struct::Iterator Struct::begin() const
    {
        return Iterator(m_data);
    }

    Struct::Iterator Struct::end() const
    {
        auto iter          = Iterator(m_data);
        iter.m_currentIter = m_data.lock()->fields.end();
        return iter;
    }
    Struct::Iterator::Iterator(std::weak_ptr<StructData> parent) :
        m_parent(parent), m_currentIter(parent.lock()->fields.begin())
    {
    }

    Struct::Iterator::reference Struct::Iterator::operator*() const
    {
        return Struct(Typed(m_currentIter->second));
    }

    Struct::Iterator::pointer Struct::Iterator::operator->()
    {
        return Struct(Typed(m_currentIter->second));
    }

    // Prefix increment
    Struct::Iterator& Struct::Iterator::operator++()
    {
        m_currentIter++;
        return *this;
    }

    // Postfix increment
    Struct::Iterator Struct::Iterator::operator++(int)
    {
        auto other = *this;
        ++(*this);
        return other;
    }

    bool operator==(const Struct::Iterator& a, const Struct::Iterator& b)
    {
        return a.m_currentIter == b.m_currentIter;
    }

    bool operator!=(const Struct::Iterator& a, const Struct::Iterator& b)
    {
        return a.m_currentIter != b.m_currentIter;
    }

    Field Struct::operator[](const std::string_view str)
    {
        if (auto val = getField(std::string(str)); val.has_value())
        {
            return val.value();
        }
        throw std::runtime_error(
            "Invalid access of struct! Use Struct::getField which "
            "uses std::optional instead if unsure");
    }

    bool Struct::containsField(const std::string_view str)
    {
        if (auto val = getField(std::string(str)); val.has_value())
        {
            return true;
        }
        return false;
    }

    bool Struct::containsField(const Field& field)
    {
        return containsField(std::string(field.name()));
    }
} // namespace rmf
