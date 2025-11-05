cmake_minimum_required(VERSION 3.23)

project(MemAnal)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_COMPILER g++)
# doesn't work ;_;
# set(CMAKE_USE_RELATIVE_PATHS True)

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

file(GLOB_RECURSE LIB_SRC_FILES "src/*.cpp")
list(FILTER LIB_SRC_FILES EXCLUDE REGEX ".*/main\\.cpp$")

file(GLOB MAIN_SRC_FILE "src/main.cpp")

FetchContent_Declare(
    imgui
    GIT_REPOSITORY    https://github.com/ocornut/imgui.git
    # GIT_TAG           v1.90.5  # Use a specific, stable tag/version
    GIT_SHALLOW       TRUE     # Faster download
    # SOURCE_DIR lets you specify where it's downloaded (optional, often derived)
)

FetchContent_MakeAvailable(imgui)

file(GLOB imguiFiles ${imgui_SOURCE_DIR}/*.cpp)

add_library(ImGuiSource STATIC
    ${imguiFiles}

    # Include the necessary backend files for SDL3 (and your renderer, e.g., OpenGL3)
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp 
)

target_include_directories(ImGuiSource PUBLIC ${imgui_SOURCE_DIR})

add_library(memLib STATIC ${LIB_SRC_FILES})

target_include_directories(memLib PUBLIC
    "include"
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(ImGuiSource PUBLIC SDL3::SDL3)

add_executable(mem-anal ${MAIN_SRC_FILE})

target_link_libraries(mem-anal PRIVATE memLib
    PRIVATE ImGuiSource
    PRIVATE SDL3::SDL3
    PRIVATE OpenGL::GL
)


enable_testing()

add_subdirectory(tests)
