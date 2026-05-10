#include "rmf/logging/logging.hpp"
#include "rmf/utils/expect.hpp"
#include <array>
#include <cassert>
#include <memory>
#include <utility>
#include "rmf/utils/other.hpp"
#define RMF_NO_CLEANUP_MACROS
#include "rmf/type_registry.hpp"
#undef RMF_NO_CLEANUP_MACROS

#include <magic_enum/magic_enum.hpp>

namespace mf  = RealtimeMemoryForensics;
namespace mfu = mf::Utils;

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

constexpr std::array<mf::PType, 11> primTypes = {
#define X(name, size) mf::PType::name##size,
    RMF_PRIM_TYPES(X)
#undef X
};

// In order to preconstruct all the stuff with deleted default constructors.
mf::TypeRegistry mf::TypeRegistry::Make()
{
    std::vector<sptr<PrimitiveData>> primitives{};
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
    .name##size = Primitive(                                                   \
        Typed(primitives[static_cast<size_t>(mf::PType::name##size)])),
        RMF_PRIM_TYPES(X)
#undef X
    };
    return mf::TypeRegistry(std::move(primitives), std::move(primList));
}

mf::TypeRegistry::TypeRegistry(std::vector<sptr<PrimitiveData>>&& primitives,
                               PrimitiveList&& primitiveList) :
    prim(primitiveList)
{
    m_data->primitives = std::move(primitives);
}

mf::StructBuilder mf::TypeRegistry::defStruct(const strview name)
{
    return StructBuilder(*this, name);
}

mf::Pointer mf::TypeRegistry::ptrTo(Typed T)
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
    return mf::Pointer(mf::Typed(ptr));
}

mf::Array mf::TypeRegistry::arrOf(Typed T, ssize_t length)
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
    return mf::Array(mf::Typed(ptr));
}

mf::Struct mf::TypeRegistry::struct_(const strview name)
{
    // I forgot how stupid is this. I have to promote it to a fucknig string.
    return mf::Struct(mf::Typed(m_data->structs.at(std::string(name))));
}

mf::StructBuilder::StructBuilder(const mf::TypeRegistry& parent,
                                 const strview           name) :
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

mf::StructBuilder&& mf::StructBuilder::field(Typed type, const strview name)
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
    ssize_t padding = (memberAlignment - currentOffset % memberAlignment);
    padding         = (padding == memberAlignment) ? 0 : padding;
    return currentOffset + padding;
}

mf::Struct mf::StructBuilder::end()
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
        field->offset = calculateOffset(cumulativeOffset, target->alignment);
        field->size   = target->size;
        cumulativeOffset += target->size;
        rmf_Info("{}:{} - offset: {}, size: {}, cumulative: {}", name,
                 target->name, field->offset, field->size, cumulativeOffset);
    }
    m_structData->size      = calculateOffset(cumulativeOffset, finalAlignment);
    m_structData->alignment = finalAlignment;
    rmf_Info("final size: {}, final alignment: {}", m_structData->size,
             m_structData->alignment);
    return mf::Struct(mf::Typed(m_structData));
}

mf::Typed::Typed(wptr<BaseTypeData> data) : m_baseData(data)
{
}

mf::Typed mf::Typed::makeFromWptr(wptr<BaseTypeData> data)
{
    return Typed(data);
}

ssize_t mf::Typed::size() const
{
    return m_baseData.lock()->size;
}

ssize_t mf::Typed::alignment() const
{
    return m_baseData.lock()->alignment;
}

mf::Type mf::Typed::type() const
{
    return m_baseData.lock()->type;
}

const mf::strview mf::Typed::name() const
{
    return m_baseData.lock()->name;
}

mf::Struct::Struct(const Typed& typed) : Typed(typed)
{
    m_data = std::static_pointer_cast<StructData>(m_baseData.lock());
}

mfu::ErrU<mf::Field> mf::Struct::getField(const std::string& str)
{
    auto data = m_data.lock();
    if (data->fields.contains(str))
    {
        return mf::Field(mf::Typed::makeFromWptr(data->fields[str]));
    }
    return rmf_mkErr(Utils::ErrorEnum::FieldDoesNotExist);
}

mf::Pointer::Pointer(const Typed& typed) : Typed(typed)
{
    m_data = std::static_pointer_cast<PointerData>(m_baseData.lock());
}

mf::Primitive::Primitive(const Typed& typed) : Typed(typed)
{
    m_data = std::static_pointer_cast<PrimitiveData>(m_baseData.lock());
}

mf::Array::Array(const Typed& typed) : Typed(typed)
{
    m_data = std::static_pointer_cast<ArrayData>(m_baseData.lock());
}

mf::Field::Field(const Typed& typed) : Typed(typed)
{
    m_data = std::static_pointer_cast<FieldData>(m_baseData.lock());
}
ssize_t mf::Field::offset() const
{
    return m_data.lock()->offset;
}

mf::Struct::Iterator mf::Struct::begin() const
{
    return Iterator(m_data);
}
mf::Struct::Iterator mf::Struct::end() const
{
    return Iterator(m_data);
}
mf::Struct::Iterator::Iterator(wptr<StructData> parent) :
    m_parent(parent), m_currentIter(parent.lock()->fields.begin())
{
}

mf::Struct::Iterator::reference mf::Struct::Iterator::operator*() const
{
    return mf::Struct(mf::Typed(m_currentIter->second));
}

mf::Struct::Iterator::pointer mf::Struct::Iterator::operator->()
{
    return mf::Struct(mf::Typed(m_currentIter->second));
}

// Prefix increment
mf::Struct::Iterator& mf::Struct::Iterator::operator++()
{
    m_currentIter++;
    return *this;
}

// Postfix increment
mf::Struct::Iterator mf::Struct::Iterator::operator++(int)
{
    auto other = *this;
    ++(*this);
    return other;
}

bool mf::operator==(const Struct::Iterator& a, const Struct::Iterator& b)
{
    return a.m_currentIter == b.m_currentIter;
}

bool mf::operator!=(const Struct::Iterator& a, const Struct::Iterator& b)
{
    return a.m_currentIter != b.m_currentIter;
}
