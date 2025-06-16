cmake_minimum_required(VERSION 3.25...3.31 FATAL_ERROR)
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
if(NOT DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT IPO_SUPPORTED)

	if(IPO_SUPPORTED)
		message(STATUS "Interprocedural Optimization (LTO): Enabled for Release builds")
	else()
		message(WARNING "Interprocedural Optimization (LTO): Not supported by compiler")
	endif()

	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION $<$<CONFIG:Release>:${IPO_SUPPORTED}>)
endif()

find_package(Vulkan REQUIRED)

# ==== Distribution Options ====
option(ZENITH_TRACK_MEMORY "Enable memory tracking" ON)
option(ZENITH_TESTS "Build Zenith tests" ON)

function(copy_resources target)
	add_custom_command(TARGET ${target} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_SOURCE_DIR}/Resources
			$<TARGET_FILE_DIR:${target}>/Resources
			COMMENT "Copying Resources to ${target}"
			VERBATIM
	)
endfunction()

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