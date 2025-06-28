# CMake Version-Feature Compatibility Table
# ===========================================
# `cmake_minimum_required(VERSION x.y...z.w)`: 3.12
# `cmake_policy(CMP0069)`/`check_ipo_supported()`: 3.9
# `cmake_policy(CMP0077)`: 3.13
# `cmake_policy(CMP0118)`: 3.20
# `cmake_policy(CMP0163)`: 3.30
# `cmake_policy(CMP0171)`: 3.31
# `CMAKE_POLICY_DEFAULT_CMP0077`: 3.13
# `CMAKE_POSITION_INDEPENDENT_CODE`: 3.4
# `BUILD_SHARED_LIBS`: 3.4
# `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`: 3.12
# `target_precompile_headers()`: 3.16
# `target_compile_features(... cxx_std_20)`: 3.12 (C++20: 3.20)
# `list(FILTER ... EXCLUDE REGEX)`: 3.6
# `list(JOIN)`: 3.12
# `list(APPEND)`: 3.0
# `target_compile_definitions(... PUBLIC/PRIVATE)`: 3.0
# `target_link_libraries(... PUBLIC/PRIVATE)`: 3.0
# `target_include_directories(... PUBLIC/PRIVATE)`: 3.0
# `add_compile_options($<$<CXX_COMPILER_ID:MSVC>:...>)`: 3.5
# `$<$<BOOL:...>:...>` generator expressions: 3.8
# `FetchContent_Declare/MakeAvailable`: 3.14
# `enable_testing()`/`gtest_discover_tests()`: 3.10
# `include(GoogleTest)`: 3.10

cmake_minimum_required(VERSION 3.25...4.0 FATAL_ERROR)
project(Zenith  LANGUAGES C CXX)

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

# ==== Interprocedural Optimization (Per-target approach) ====
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED)

if(IPO_SUPPORTED)
	message(STATUS "Interprocedural Optimization (LTO): Available for per-target use")
else()
	message(WARNING "Interprocedural Optimization (LTO): Not supported by compiler")
endif()

# ==== CMake Utilities ====
include(BuildSystem/CMake/CMakeUtils/znUtilsTarget.cmake)

find_package(Vulkan REQUIRED)

# ==== Distribution Options ====
option(ZENITH_TRACK_MEMORY "Enable memory tracking" ON)
option(ZENITH_TESTS "Build Zenith tests" ON)

add_compile_options(
		$<$<CXX_COMPILER_ID:MSVC>:/Gy>
		$<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>
)

# ==== Add Subsystems ====
add_subdirectory(ThirdParty)
add_subdirectory(Engine)
add_subdirectory(Editor)

if(ZENITH_TESTS)
	enable_testing()
	add_subdirectory(Tests)
endif()