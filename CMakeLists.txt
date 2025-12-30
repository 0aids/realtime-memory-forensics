cmake_minimum_required(VERSION 3.23)

project(realtime_memory_forensics)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS 
    "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=. -fdiagnostics-show-template-tree"
)

##################################################################
##################### External library stuff #####################
##################################################################

include(FetchContent)

add_compile_options(-g -Wall -Wextra -Wno-cpp -Wpedantic --warn-no-format-security)

option(DisableOPT "Disable optimization using -O0" OFF)
if (DisableOPT)
    add_compile_options(-O0)
else()
    add_compile_options(-O3 -DIMGUI_DISABLE_DEMO_WINDOWS)
endif()

find_package(SDL3 REQUIRED)
find_package(OpenGL REQUIRED)


FetchContent_Declare(
    imgui
    GIT_REPOSITORY    https://github.com/ocornut/imgui.git
    GIT_TAG           v1.92.5
    GIT_SHALLOW       TRUE     
)

FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_SHALLOW TRUE
    GIT_TAG        v0.9.7
)


FetchContent_MakeAvailable(magic_enum)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_SHALLOW TRUE
    GIT_TAG v3.0.1
)

FetchContent_MakeAvailable(pybind11)

pybind11_add_module(rmf_py
    bindings/rmf_py_bindings.cpp
)

target_link_libraries(rmf_py
    PRIVATE memLib
    PRIVATE ImGuiSource
    PRIVATE SDL3::SDL3
    PRIVATE OpenGL::GL
    PRIVATE magic_enum::magic_enum
)

target_include_directories(rmf_py PUBLIC "include")

file(GLOB imguiFiles ${imgui_SOURCE_DIR}/*.cpp)

add_library(ImGuiSource STATIC
    ${imguiFiles}

    # Include the necessary backend files for SDL3 (and your renderer, e.g., OpenGL3)
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp 
)

target_include_directories(ImGuiSource PUBLIC ${imgui_SOURCE_DIR})

target_link_libraries(ImGuiSource PUBLIC SDL3::SDL3)

############################################################
##################### rmf source files #####################
############################################################

file(GLOB_RECURSE LIB_SRC_FILES "src/*.cpp")
list(FILTER LIB_SRC_FILES EXCLUDE REGEX ".*/main\\.cpp$")

file(GLOB MAIN_SRC_FILE "src/main.cpp")

add_library(memLib STATIC ${LIB_SRC_FILES})

target_include_directories(memLib PUBLIC
    "include"
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${magic_enum_SOURCE_DIR}/include
)

add_executable(mem-anal ${MAIN_SRC_FILE})

target_link_libraries(mem-anal 
    PRIVATE memLib
    PRIVATE ImGuiSource
    PRIVATE SDL3::SDL3
    PRIVATE OpenGL::GL
    PRIVATE magic_enum::magic_enum
)

option(BuildTests "Build the tests" ON)
if (BuildTests)
    enable_testing()

    add_subdirectory(tests)
endif()
