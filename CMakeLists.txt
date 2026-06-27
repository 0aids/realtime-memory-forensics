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
    -ggdb
)
set(CXX_COMP_COMMANDS
    -Wall -Wextra -Wpedantic -Wno-cpp
    -fdiagnostics-show-template-tree
    -fno-omit-frame-pointer
    -g3
    $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O3>
)

option(Asan "Address sanitizer" ON)
option(UBsan "Undefined behaviour sanitizer" OFF)
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

# ---------------- rmf ----------------
file(GLOB_RECURSE rmf_src_files "src/*")
add_library(rmf STATIC ${rmf_src_files})
target_link_libraries(rmf PUBLIC
	magic_enum::magic_enum
)
target_compile_options(rmf PUBLIC ${CXX_COMP_COMMANDS})
target_link_libraries(rmf PUBLIC ${CXX_LINK_COMMANDS})
target_include_directories(rmf PUBLIC "include")

if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

find_package(Python 3.9 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)
# Detect the installed nanobind package and import it into CMake
execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT)
find_package(nanobind CONFIG REQUIRED)

set(bindings_cpps "bindings/rmf.cpp")
nanobind_add_module(rmfpy NOMINSIZE ${bindings_cpps})
target_compile_options(rmfpy PRIVATE ${CXX_COMP_COMMANDS})
target_compile_options(rmfpy PRIVATE $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O3>)
target_link_libraries(rmfpy PRIVATE rmf)

option(tests "Compile tests" ON)
if(tests)
	enable_testing()
    add_subdirectory("tests")
endif()
