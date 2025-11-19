set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

file(GLOB_RECURSE TEST_FILES "*.cpp")

file(GLOB_RECURSE TEST_SCRIPT_FILES "*.rmf")
file(COPY ${TEST_SCRIPT_FILES} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

foreach(TEST_FILE ${TEST_FILES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE memLib)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

