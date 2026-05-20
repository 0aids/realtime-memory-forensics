namespace rmf
{
    namespace Detail
    {
        Perms parsePerms(const std::string_view perms);
        struct MapData
        {
            static std::shared_ptr<const std::string> defaultName;
            // Default values for safety
            uintptr_t                          parentAddress   = 0;
            uintptr_t                          parentSize      = 0;
            ptrdiff_t                          relativeAddress = 0;
            ptrdiff_t                          relativeSize    = 0;
            std::shared_ptr<const std::string> regionName_sp   = defaultName;
            Perms                              perms           = Perms::None;
            bool operator==(const MapData& other) const        = default;
        };
    }
}

