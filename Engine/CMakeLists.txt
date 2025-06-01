set(ZENITH_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

file(GLOB_RECURSE ZENITH_SOURCES CONFIGURE_DEPENDS
		${ZENITH_SRC_DIR}/Zenith/*.cpp
		${ZENITH_SRC_DIR}/Zenith/*.hpp
		${ZENITH_SRC_DIR}/*.c
		${ZENITH_SRC_DIR}/*.h
)

add_library(Zenith STATIC)

target_sources(Zenith PRIVATE ${ZENITH_SOURCES})

target_precompile_headers(Zenith PRIVATE ${ZENITH_SRC_DIR}/znpch.hpp)

target_compile_features(Zenith PUBLIC cxx_std_20)

if (MSVC)
		target_compile_options(Zenith PRIVATE /Zc:preprocessor)
endif()

target_include_directories(Zenith PUBLIC ${ZENITH_SRC_DIR})

add_definitions(-DSDL_MAIN_HANDLED)
target_link_libraries(Zenith
	PUBLIC
		SDL3::SDL3-static
		spdlog
		glm
		glad
		Tracy::TracyClient
		backward
)
