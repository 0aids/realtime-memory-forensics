#include "region_properties.hpp"
#include <sstream>
#include <string>
#include <string_view>

std::string permissionsMaskToString(const PermissionsMask& perms) {
#define R 0
#define W 1
#define X 2
#define P 3
    std::string mask = "....";
    if ((perms & READ) != 0) {
        mask[R] = 'r';
    }
    if ((perms & WRITE) != 0) {
        mask[W] = 'w';
    }
    if ((perms & EXECUTE) != 0) {
        mask[X] = 'x';
    }
    if ((perms & PRIVATE) != 0) {
        mask[P] = 'p';
    }
#undef R
#undef W
#undef X
#undef P

    return mask;
}
PermissionsMask toPermissionsMask(const char perms[4]) {
#define R 0
#define W 1
#define X 2
#define P 3
    PermissionsMask mask = 0;
    if (perms[R] == 'r') {
        mask |= READ;
    }
    if (perms[W] == 'w') {
        mask |= WRITE;
    }
    if (perms[X] == 'x') {
        mask |= EXECUTE;
    }
    if (perms[P] == 'p') {
        mask |= PRIVATE;
    }
#undef R
#undef W
#undef X
#undef P

    return mask;
}
PermissionsMask toPermissionsMask(const std::string& chars) {
    PermissionsMask mask = 0;
    if (chars.contains('r')) {
        mask |= READ;
    }
    if (chars.contains('w')) {
        mask |= WRITE;
    }
    if (chars.contains('x')) {
        mask |= EXECUTE;
    }
    if (chars.contains('p')) {
        mask |= PRIVATE;
    }
    return mask;
}

PermissionsMask toPermissionsMask(const std::string_view& chars) {
    PermissionsMask mask = 0;
    if (chars.contains('r')) {
        mask |= READ;
    }
    if (chars.contains('w')) {
        mask |= WRITE;
    }
    if (chars.contains('x')) {
        mask |= EXECUTE;
    }
    if (chars.contains('p')) {
        mask |= PRIVATE;
    }
    return mask;
}

MemoryRegionProperties::MemoryRegionProperties(
    std::string name, uintptr_t start, size_t size,
    PermissionsMask perms) :
    parentRegionName(name), parentRegionStart(start),
    parentRegionSize(size), permissions(perms) {

    regionStart = 0;
    regionSize  = parentRegionSize;
}

std::string MemoryRegionProperties::toStr() {
    std::stringstream str;
    str << "ParentRegionName: " << parentRegionName << "\n";
    str << "ParentRegionStart: " << std::showbase << std::hex
        << parentRegionStart << "\n";
    str << "ParentRegionSize: " << std::showbase << std::hex
        << parentRegionSize << "\n";
    str << "Perms: " << permissionsMaskToString(permissions) << "\n";
    str << "\n";
    str << "Start: " << regionStart << "\n";
    str << "Size: " << regionSize << "\n";
    return str.str();
}
const std::string MemoryRegionProperties::toConstStr() const {
    std::stringstream str;
    str << "ParentRegionName: " << parentRegionName << "\n";
    str << "ParentRegionStart: " << std::showbase << std::hex
        << parentRegionStart << "\n";
    str << "ParentRegionSize: " << std::showbase << std::hex
        << parentRegionSize << "\n";
    str << "Perms: " << permissionsMaskToString(permissions) << "\n";
    str << "\n";
    str << "Start: " << regionStart << "\n";
    str << "Size: " << regionSize << "\n";
    return str.str();
}

std::ostream& operator<<(std::ostream&                 os,
                         const MemoryRegionProperties& properties) {
    os << properties.toConstStr();
    return os;
}

RegionPropertiesList RegionPropertiesList::getRegionsWithPermissions(
    const std::string_view& chars) {
    RegionPropertiesList list{};
    auto                 mask = toPermissionsMask(chars);

    // Going to be slow cause we gotta make copies.
    for (const MemoryRegionProperties& p : *this) {
        // Log_f(Debug,
        //       "p.permissions: " << p.permissions
        //                         << "\tmask: " << mask);
        if (p.permissions == mask) {
            list.push_back(p);
        }
    }
    return list;
}
RegionPropertiesList RegionPropertiesList::getRegionsWithPermissions(
    const PermissionsMask& mask) {
    RegionPropertiesList list{};

    // Going to be slow cause we gotta make copies.
    for (const MemoryRegionProperties& p : *this) {
        // Log_f(Debug,
        //       "p.permissions: " << p.permissions
        //                         << "\tmask: " << mask);
        if (p.permissions == mask) {
            list.push_back(p);
        }
    }
    return list;
}
