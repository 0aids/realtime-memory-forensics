#ifndef helpers_hpp_INCLUDED
#define helpers_hpp_INCLUDED

// Helper to ensure that compilation of existant member succeeds
#define comp_succ(message, ...)                                      \
    static_assert(requires { __VA_ARGS__; }, message)
// Helper to ensure that compilation of existant member fails
#define comp_fail(message, ...)                                      \
    static_assert(not requires { __VA_ARGS__; }, message)

#endif // helpers_hpp_INCLUDED
