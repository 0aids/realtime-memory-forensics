#ifndef builds_hpp_INCLUDED
#define builds_hpp_INCLUDED

#include <atomic>

template <typename ResultType>
struct BuildJob
{
    // private:
    ResultType::Builder build;

  public:
    // The result is moved.
    inline ResultType getResult()
    {
        return build.build();
    }

    // Forward any arguments required to build.
    template <typename... Args>
    BuildJob(Args&&... args) : build(std::forward<Args>(args)...){};
};

#endif // builds_hpp_INCLUDED
