find_package(GTest REQUIRED)
include(GoogleTest)

file(GLOB testFiles_cpp "*.cpp")
set(testNames)

foreach(testFile_cpp ${testFiles_cpp})
    get_filename_component(testName ${testFile_cpp} NAME_WE)
	add_executable(${testName} ${testFile_cpp})
	target_link_libraries(${testName} 
        PRIVATE memLib
        PRIVATE ImGuiSource
        PRIVATE SDL3::SDL3
        PRIVATE OpenGL::GL
        PRIVATE magic_enum::magic_enum
		PUBLIC GTest::gtest_main
	)
    gtest_discover_tests(${testName})
endforeach()

