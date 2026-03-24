cmake_minimum_required(VERSION 3.23)
project(realtime_memory_forensics LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COLOR_DIAGNOSTICS ON)


include(FetchContent)
add_compile_definitions("PYBIND11_DETAILED_ERROR_MESSAGES")

# ---------------- Build type ----------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(GLOBAL_WARNINGS
    -Wall -Wextra -Wpedantic -Wno-cpp --warn-no-format-security
    -fdiagnostics-show-template-tree
    -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=. -g
)

# ---------------- Sanitizers ----------------
option(asan_address "Enable AddressSanitizer" OFF)
if(asan_address)
    message(WARNING "RMF: AddressSanitizer enabled (Causes tests to fail spuriously!)")
    set(SAN_FLAGS -fsanitize=address)
endif()

option(asan_undefined "Enable Undefined santisizer" OFF)
if(asan_undefined)
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

add_library(libImgui STATIC
    ${imgui_core}
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(libImgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(libImgui PUBLIC SDL3::SDL3 OpenGL::GL)
target_compile_options(libImgui PRIVATE ${GLOBAL_WARNINGS})
target_compile_options(libImgui PRIVATE -O3)
target_compile_options(libImgui PRIVATE ${SAN_FLAGS})
target_link_options(libImgui PRIVATE ${SAN_FLAGS})

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
    PRIVATE librmf magic_enum::magic_enum
)
target_compile_definitions(rmf_core_py PUBLIC "PYBIND11_DETAILED_ERROR_MESSAGES")
set_target_properties(rmf_core_py PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/rmf_py/"
)
target_compile_options(rmf_core_py PRIVATE ${GLOBAL_WARNINGS})
target_compile_options(rmf_core_py PRIVATE $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O3>)
target_compile_options(rmf_core_py PRIVATE ${SAN_FLAGS})
target_compile_options(rmf_core_py PUBLIC -fdiagnostics-show-template-tree)
target_link_options(rmf_core_py PRIVATE ${SAN_FLAGS})

pybind11_add_module(rmf_test_py bindings/rmf_py_tests.cpp)
target_compile_definitions(rmf_test_py PUBLIC "PYBIND11_DETAILED_ERROR_MESSAGES")
target_include_directories(rmf_test_py PRIVATE include)
target_link_libraries(rmf_test_py
    PRIVATE librmf magic_enum::magic_enum
)
set_target_properties(rmf_test_py PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/rmf_py/"
)
target_compile_options(rmf_test_py PRIVATE ${GLOBAL_WARNINGS})
target_compile_options(rmf_test_py PRIVATE $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O3>)
target_compile_options(rmf_test_py PRIVATE ${SAN_FLAGS})
target_link_options(rmf_test_py PRIVATE ${SAN_FLAGS})

# ---------------- imnodes ----------------
file(GLOB imnodes_cpp external/imnodes/*.cpp)

add_library(libImnodes STATIC ${imnodes_cpp})
target_include_directories(libImnodes PUBLIC external/imnodes)
target_link_libraries(libImnodes PRIVATE libImgui)
target_compile_options(libImnodes PRIVATE ${GLOBAL_WARNINGS})
target_compile_options(libImnodes PRIVATE -O3)
target_compile_options(libImnodes PRIVATE ${SAN_FLAGS})
target_link_options(libImnodes PRIVATE ${SAN_FLAGS})

# ---------------- logger ----------------
file(GLOB_RECURSE librmf_logger_src_cpp logger/*.cpp)

add_library(librmf_logger STATIC ${librmf_logger_src_cpp})
target_include_directories(librmf_logger PUBLIC
    logger
)

target_link_libraries(librmf_logger PRIVATE 
	magic_enum::magic_enum
)
target_compile_options(librmf_logger PRIVATE ${GLOBAL_WARNINGS})
target_compile_options(librmf_logger PRIVATE $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O3>)
target_compile_options(librmf_logger PRIVATE ${SAN_FLAGS})
target_link_options(librmf_logger PRIVATE ${SAN_FLAGS})


# ---------------- librmf ----------------

file(GLOB_RECURSE librmf_src_cpp src/*.cpp)

add_library(librmf STATIC ${librmf_src_cpp})
target_include_directories(librmf PUBLIC
    include
)

target_link_libraries(librmf 
	PUBLIC magic_enum::magic_enum
	PUBLIC librmf_logger
)
target_compile_options(librmf PRIVATE ${GLOBAL_WARNINGS})
target_compile_options(librmf PRIVATE $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O3>)
target_compile_options(librmf PRIVATE ${SAN_FLAGS})
target_link_options(librmf PRIVATE ${SAN_FLAGS})

# ---------------- python embed ----------------

file(GLOB_RECURSE librmf_pythonEmbed_src_cpp python_embed/*.cpp)
add_library(librmf_pythonEmbed STATIC ${librmf_pythonEmbed_src_cpp})

target_include_directories(librmf_pythonEmbed PUBLIC
	python_embed/
)

target_link_libraries(librmf_pythonEmbed PRIVATE 
	magic_enum::magic_enum
	pybind11::embed
	librmf_logger
)
target_compile_options(librmf_pythonEmbed PRIVATE ${GLOBAL_WARNINGS})
target_compile_options(librmf_pythonEmbed PRIVATE $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O3>)
target_compile_options(librmf_pythonEmbed PRIVATE ${SAN_FLAGS})
target_link_options(librmf_pythonEmbed PRIVATE ${SAN_FLAGS})

# ---------------- RMF gui ----------------
option(build_gui "build the gui" OFF)
if (build_gui)
    file(GLOB_RECURSE rmf_gui_cpp gui/src/*.cpp)
    add_executable(rmf_gui ${rmf_gui_cpp})
    target_include_directories(rmf_gui  PUBLIC
        gui/include
    )
    target_link_libraries(rmf_gui PRIVATE 
        magic_enum::magic_enum
    	libImgui
    	SDL3::SDL3
    	OpenGL::GL
    	libImnodes
    	pybind11::embed
        librmf
    	# fix for py embedded modules not being linked properly.
        "$<LINK_LIBRARY:WHOLE_ARCHIVE,librmf_pythonEmbed>"
    	# Because the functions generated by the PYBIND11_EMBEDDED_MODULE
    	# macro were not directly used, they were discarded so when 
    	# being called via a string call for importing it crashed.
    )
endif()

# ---------------- Tests ----------------
option(build_tests "compile tests" ON)
if(build_tests)
    enable_testing()
    add_subdirectory(tests)
endif()

option(enableExamples "Compile example executables" ON)
if(enableExamples)
    file(GLOB exampleFiles examples/*.cpp)
    foreach(exampleFile_cpp ${exampleFiles})
    	get_filename_component(exampleName ${exampleFile_cpp} NAME_WE)
    	add_executable(${exampleName} ${exampleFile_cpp})
        target_include_directories(${exampleName} PUBLIC external/imnodes)
        target_link_libraries(${exampleName} PUBLIC libImgui)
        target_compile_options(${exampleName} PRIVATE ${GLOBAL_WARNINGS})
        target_compile_options(${exampleName} PRIVATE $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O3>)
        target_compile_options(${exampleName} PRIVATE ${SAN_FLAGS})
        target_link_options(${exampleName} PRIVATE ${SAN_FLAGS})
        target_link_libraries(${exampleName} PRIVATE 
        	libImgui
        	SDL3::SDL3
        	OpenGL::GL
        	libImnodes
            pybind11::embed
			librmf
        	# fix for py embedded modules not being linked properly.
            "$<LINK_LIBRARY:WHOLE_ARCHIVE,librmf_pythonEmbed>"
        	# Because the functions generated by the PYBIND11_EMBEDDED_MODULE
        	# macro were not directly used, they were discarded so when 
        	# being called via a string call for importing it crashed.
        )
    endforeach()
endif()
