cmake_minimum_required(VERSION 3.23)
project(realtime_memory_forensics LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
add_compile_definitions("PYBIND11_DETAILED_ERROR_MESSAGES")

# ---------------- Build type ----------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(GLOBAL_WARNINGS
    -Wall -Wextra -Wpedantic -Wno-cpp --warn-no-format-security
    -fdiagnostics-show-template-tree
    -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=. -g -O0
)

# ---------------- Sanitizers ----------------
if(DEFINED asan_address)
    message(WARNING "RMF: AddressSanitizer enabled (RAM heavy!)")
    set(SAN_FLAGS -fsanitize=address)
endif()

if(DEFINED asan_undefined)
    message("RMF: UndefinedBehaviorSanitizer enabled")
    list(APPEND SAN_FLAGS -fsanitize=undefined)
endif()

# ---------------- External libs ----------------
find_package(SDL3 REQUIRED)
find_package(OpenGL REQUIRED)

# ---------------- ImGui ----------------
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.6-docking
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

file(GLOB imgui_core ${imgui_SOURCE_DIR}/*.cpp)

add_library(ImGuiSource STATIC
    ${imgui_core}
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(ImGuiSource PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(ImGuiSource PUBLIC SDL3::SDL3 OpenGL::GL)
target_compile_options(ImGuiSource PRIVATE ${GLOBAL_WARNINGS})
target_compile_options(ImGuiSource PRIVATE $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O2>)
target_compile_options(ImGuiSource PRIVATE ${SAN_FLAGS})
target_link_options(ImGuiSource PRIVATE ${SAN_FLAGS})

# ---------------- magic_enum ----------------
FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG v0.9.7
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(magic_enum)

# ---------------- pybind11 ----------------
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v3.0.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(pybind11)

pybind11_add_module(rmf_core_py bindings/rmf_py_bindings.cpp)
target_include_directories(rmf_core_py PRIVATE include)
target_link_libraries(rmf_core_py
    PRIVATE memLib ImGuiSource SDL3::SDL3 OpenGL::GL magic_enum::magic_enum
)
target_compile_definitions(rmf_core_py PUBLIC "PYBIND11_DETAILED_ERROR_MESSAGES")
set_target_properties(rmf_core_py PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/rmf_py/"
)
target_compile_options(rmf_core_py PRIVATE ${GLOBAL_WARNINGS})
target_compile_options(rmf_core_py PRIVATE $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O2>)
target_compile_options(rmf_core_py PRIVATE ${SAN_FLAGS})
target_link_options(rmf_core_py PRIVATE ${SAN_FLAGS})

pybind11_add_module(rmf_test_py bindings/rmf_py_tests.cpp)
target_compile_definitions(rmf_test_py PUBLIC "PYBIND11_DETAILED_ERROR_MESSAGES")
target_include_directories(rmf_test_py PRIVATE include)
target_link_libraries(rmf_test_py
    PRIVATE memLib ImGuiSource SDL3::SDL3 OpenGL::GL magic_enum::magic_enum
)
set_target_properties(rmf_test_py PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/rmf_py/"
)
target_compile_options(rmf_test_py PRIVATE ${GLOBAL_WARNINGS})
target_compile_options(rmf_test_py PRIVATE $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O2>)
target_compile_options(rmf_test_py PRIVATE ${SAN_FLAGS})
target_link_options(rmf_test_py PRIVATE ${SAN_FLAGS})

# ---------------- imnodes ----------------
file(GLOB imnodes_cpp external/imnodes/*.cpp)

add_library(libImnodes STATIC ${imnodes_cpp})
target_include_directories(libImnodes PUBLIC external/imnodes)
target_link_libraries(libImnodes PUBLIC ImGuiSource)
target_compile_options(libImnodes PRIVATE ${GLOBAL_WARNINGS})
target_compile_options(libImnodes PRIVATE $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O2>)
target_compile_options(libImnodes PRIVATE ${SAN_FLAGS})
target_link_options(libImnodes PRIVATE ${SAN_FLAGS})

# ---------------- RMF sources ----------------
file(GLOB_RECURSE LIB_SRC_FILES src/*.cpp)
list(FILTER LIB_SRC_FILES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(memLib STATIC ${LIB_SRC_FILES})
target_include_directories(memLib PUBLIC
    include
    external/imnodes
)

target_link_libraries(memLib PUBLIC 
	magic_enum::magic_enum
	ImGuiSource
	libImnodes
    pybind11::embed
)
target_compile_options(memLib PRIVATE ${GLOBAL_WARNINGS})
target_compile_options(memLib PRIVATE $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O2>)
target_compile_options(memLib PRIVATE ${SAN_FLAGS})
target_link_options(memLib PRIVATE ${SAN_FLAGS})

add_executable(rmf_exe src/main.cpp)
target_link_libraries(rmf_exe PRIVATE 
	ImGuiSource
	SDL3::SDL3
	OpenGL::GL
	libImnodes
	# fix for py embedded modules not being linked properly.
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,memLib>"
	# Because the functions generated by the PYBIND11_EMBEDDED_MODULE
	# macro were not directly used, they were discarded so when 
	# being called via a string call for importing it crashed.
)

# ---------------- Tests ----------------
if(DEFINED build_tests)
    enable_testing()
    add_subdirectory(tests)
endif()

file(GLOB exeFiles exes/*.cpp)
foreach(exeFile_cpp ${exeFiles})
	get_filename_component(exeName ${exeFile_cpp} NAME_WE)
	add_executable(${exeName} ${exeFile_cpp})
    target_include_directories(${exeName} PUBLIC external/imnodes)
    target_link_libraries(${exeName} PUBLIC ImGuiSource)
    target_compile_options(${exeName} PRIVATE ${GLOBAL_WARNINGS})
    target_compile_options(${exeName} PRIVATE $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O2>)
    target_compile_options(${exeName} PRIVATE ${SAN_FLAGS})
    target_link_options(${exeName} PRIVATE ${SAN_FLAGS})
    target_link_libraries(${exeName} PRIVATE 
    	ImGuiSource
    	SDL3::SDL3
    	OpenGL::GL
    	libImnodes
    	# fix for py embedded modules not being linked properly.
        "$<LINK_LIBRARY:WHOLE_ARCHIVE,memLib>"
    	# Because the functions generated by the PYBIND11_EMBEDDED_MODULE
    	# macro were not directly used, they were discarded so when 
    	# being called via a string call for importing it crashed.
    )
endforeach()
