find_package(GTest REQUIRED)
include(GoogleTest)

file(GLOB_RECURSE testFiles_cpp "*.cpp")
set(testNames)

foreach(testFile_cpp ${testFiles_cpp})
    get_filename_component(testName ${testFile_cpp} NAME_WE)
	add_executable(${testName} ${testFile_cpp})
    target_compile_options(${testName} PRIVATE ${GLOBAL_WARNINGS})
    target_compile_options(${testName} PRIVATE -O0 -g)
    target_compile_options(${testName} PRIVATE ${SAN_FLAGS})
    target_link_options(${testName} PRIVATE ${SAN_FLAGS})
	target_link_libraries(${testName} 
        PRIVATE librmf
        PRIVATE SDL3::SDL3
        PRIVATE OpenGL::GL
        PRIVATE magic_enum::magic_enum
		PRIVATE pybind11::embed
        "$<LINK_LIBRARY:WHOLE_ARCHIVE,librmf_pythonEmbed>"
	
		PUBLIC GTest::gtest_main
	)
	# Solve package discoverability issues when testing
    gtest_discover_tests(${testName}
        PROPERTIES 
            ENVIRONMENT "PYTHONPATH=${CMAKE_SOURCE_DIR}"
    )
endforeach()

