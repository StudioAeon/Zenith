cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0069)
	cmake_policy(SET CMP0069 NEW)
endif()

if(POLICY CMP0077)
	cmake_policy(SET CMP0077 NEW)
endif()

set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

include(FetchContent)

set(CMAKE_WARN_DEPRECATED OFF)

FetchContent_Declare(
		googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG v1.17.0
)

FetchContent_Declare(
		benchmark
		GIT_REPOSITORY https://github.com/google/benchmark.git
		GIT_TAG v1.9.4
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark testing")

set(BUILD_GMOCK ON CACHE BOOL "")
set(INSTALL_GTEST OFF CACHE BOOL "")

FetchContent_MakeAvailable(googletest benchmark)

set(CMAKE_WARN_DEPRECATED ON)

# ==== UNIT TESTS ====
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
		"Core/*.cpp"
		"Core/*.hpp"
)

add_executable(ZenithTests ${TEST_SOURCES})

target_compile_features(ZenithTests PRIVATE cxx_std_20)

target_link_libraries(ZenithTests
		PRIVATE
		Zenith
		gtest_main
		gmock_main
)

target_include_directories(ZenithTests PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		../Engine/Source
)

include(GoogleTest)
gtest_discover_tests(ZenithTests)

# ==== CUSTOM TARGETS ====
add_custom_target(run-tests
		COMMAND ZenithTests
		DEPENDS ZenithTests
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(test-verbose
		COMMAND ZenithTests --gtest_verbose
		DEPENDS ZenithTests
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(test-xml
		COMMAND ZenithTests --gtest_output=xml:test_results.xml
		DEPENDS ZenithTests
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)