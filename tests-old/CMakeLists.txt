set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

file(GLOB_RECURSE TEST_CPP_FILES "*.cpp")

foreach(TEST_FILE ${TEST_CPP_FILES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_include_directories(${TEST_NAME} PUBLIC
        "include"
    )
    target_link_libraries(${TEST_NAME}
        PRIVATE memLib
        PRIVATE ImGuiSource
        PRIVATE SDL3::SDL3
        PRIVATE OpenGL::GL
        PRIVATE magic_enum::magic_enum
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
