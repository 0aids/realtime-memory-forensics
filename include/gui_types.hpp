#ifndef GUI_TYPES_HPP
#define GUI_TYPES_HPP

#include <string>
#include "types.hpp"

namespace rmf::gui
{

    enum class ScanOperation
    {
        ExactValue,
        Changed,
        Unchanged,
        String,
        PointerToRegion
    };

    enum class ScanValueType
    {
        i8,
        i16,
        i32,
        i64,
        u8,
        u16,
        u32,
        u64,
        f32,
        f64,
        string
    };

    struct ScanResult
    {
        uintptr_t                     address;
        std::string                   valueStr;
        types::MemoryRegionProperties mrp;
    };

}

#endif // GUI_TYPES_HPP
