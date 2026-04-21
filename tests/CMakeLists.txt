find_package(GTest REQUIRED)
include(GoogleTest)

file(GLOB_RECURSE tests_files_cpp "*.cpp")
message("-- Tests: ${test_file_cpp}")
foreach(test_file_cpp ${tests_files_cpp})
    get_filename_component(testName ${test_file_cpp} NAME_WE)
	add_executable(${testName} ${test_file_cpp})
    target_compile_options(${testName} PRIVATE ${CXX_COMP_COMMANDS})
	target_link_libraries(${testName} 
        PRIVATE librmf
		PUBLIC GTest::gtest_main
	)
endforeach()

