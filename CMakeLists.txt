cmake_minimum_required(VERSION 3.24...3.31 FATAL_ERROR)
project(Zenith CXX)

# ==== Modern CMake Policies ====
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.31")
		cmake_policy(SET CMP0171 NEW)
		cmake_policy(SET CMP0163 NEW)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.30")
		cmake_policy(SET CMP0163 NEW)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.20")
		cmake_policy(SET CMP0118 NEW)
endif()

cmake_policy(SET CMP0069 NEW)
cmake_policy(SET CMP0077 NEW)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# ==== Build Config ====
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF)

# ==== Interprocedural Optimization ====
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${IPO_SUPPORTED})

find_package(Vulkan REQUIRED)

# ==== Distribution Options ====
option(ZENITH_TRACK_MEMORY "Enable memory tracking (disabled in Dist builds)" ON)

# ==== Add Subsystems ====
add_subdirectory(ThirdParty)
add_subdirectory(Engine)
add_subdirectory(Editor)

option(ZENITH_TESTS "Build Zenith tests" ON)

if(ZENITH_TESTS)
	enable_testing()
	add_subdirectory(Tests)
endif()