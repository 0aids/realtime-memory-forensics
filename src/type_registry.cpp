#include "rmf/utils/expect.hpp"
#include <array>
#include <memory>
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
#define X(name, size) #name,
    RMF_PRIM_TYPES(X)
#undef X
};

constexpr std::array<mf::PType, 11> primTypes = {
#define X(name, size) mf::PType::name##size,
    RMF_PRIM_TYPES(X)
#undef X
};

mf::TypeRegistry::TypeRegistry()
{
    // Initialise all the primitive types.
    for (size_t i = 0; i < primTypes.size(); i++)
    {
        m_data->primitives.emplace_back(std::make_shared<PrimitiveData>(
            BaseTypeData{
                .size      = primSizes[i],
                .alignment = primSizes[i],
                .name      = primStr[i],
                .type      = Type::Primitive,
            },
            primTypes[i]));
    }
    // Define the all types.
#define X(name, size)                                                          \
    prim.name##size = Primitive(Typed(                                         \
        m_data->primitives[static_cast<size_t>(mf::PType::name##size)]));
    RMF_PRIM_TYPES(X);
#undef X
}

mf::Typed::Typed(wptr<BaseTypeData> data) : m_baseData(data)
{
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
    m_data = std::dynamic_pointer_cast<StructData>(m_baseData.lock());
}

mfu::ErrU<mf::Field> mf::Struct::getField(const std::string& str)
{
    auto data = m_data.lock();
    if (data->fields.contains(str))
    {
        return mf::Field(data->fields[str]);
    }
    return rmf_mkErr(Utils::ErrorEnum::FieldDoesNotExist);
}

mf::Pointer::Pointer(const Typed& typed) : Typed(typed)
{
    m_data = std::dynamic_pointer_cast<PointerData>(m_baseData.lock());
}

mf::Primitive::Primitive(const Typed& typed) : Typed(typed)
{
    m_data = std::dynamic_pointer_cast<PrimitiveData>(m_baseData.lock());
}

mf::Array::Array(const Typed& typed) : Typed(typed)
{
    m_data = std::dynamic_pointer_cast<ArrayData>(m_baseData.lock());
}
