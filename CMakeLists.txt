cmake_minimum_required(VERSION 4.0)
project(RealtimeMemoryForensics LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COLOR_DIAGNOSTICS ON)
add_compile_options(
    -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=. 
    -fdiagnostics-color=always
    -fno-omit-frame-pointer
    -g3
)
add_link_options(-g3)
set(CXX_COMP_COMMANDS
    -Wall -Wextra -Wpedantic -Wno-cpp
    -fdiagnostics-show-template-tree
    -fno-omit-frame-pointer
    -g3
    $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O3>
)
set(CXX_LINK_COMMANDS)

option(Asan "Address sanitizer" ON)
option(UBsan "Undefined behaviour sanitizer" ON)
if(Asan)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

if(UBsan)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()

include(FetchContent)
# ---------------- magic_enum ----------------
FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG v0.9.7
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(magic_enum)

file(GLOB_RECURSE librmf_src_files "./src/*.cpp")
message("-- librmf src files: ${librmf_src_files}")
message("-- CXX_COMP_COMMANDS: ${CXX_COMP_COMMANDS}")
message("-- CXX_LINK_COMMANDS: ${CXX_LINK_COMMANDS}")
add_library(librmf STATIC ${librmf_src_files})
target_link_libraries(librmf PRIVATE
    magic_enum::magic_enum
)
target_compile_options(librmf PUBLIC ${CXX_COMP_COMMANDS})
target_link_options(librmf PUBLIC ${CXX_LINK_COMMANDS})
target_include_directories(librmf PUBLIC "include")

option(tests "Compile tests" ON)
if(tests)
    enable_testing()
    add_subdirectory(tests)
endif()
