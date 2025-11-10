#ifndef numerics_hpp_INCLUDED
#define numerics_hpp_INCLUDED
#include <cstdint>

namespace rmf::data {
    enum class NumType {
        I8, I16, I32, I64,
        U8, U16, U32, U64,
        FLT, DLE,
    };
    struct NumQuery {
        NumType type;
        union {
            int8_t i8;
            int16_t i16;
            int32_t i32;
            int64_t i64;
            uint8_t u8;
            uint16_t u16;
            uint32_t u32;
            uint64_t u64;
            float flt;
            double dle;
        } value, min, max;
    };
}

#endif // numerics_hpp_INCLUDED
