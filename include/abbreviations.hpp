#ifndef abbreviations_hpp_INCLUDED
#define abbreviations_hpp_INCLUDED

#include <memory>

// Abbreviations, I'm fucking sick of typing long ass
// shared_ptr and unique_ptr and weak_ptr and shit like that.
namespace rmf::abv {
    template <typename T>
    using sptr = std::shared_ptr<T>;

    template <typename T>
    using wptr = std::weak_ptr<T>;

    template <typename T>
    using uptr = std::unique_ptr<T>;
}

#endif // abbreviations_hpp_INCLUDED
