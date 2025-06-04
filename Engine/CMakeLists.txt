set(ZENITH_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

file(GLOB_RECURSE ZENITH_SOURCES CONFIGURE_DEPENDS
		${ZENITH_SRC_DIR}/Zenith/*.cpp
		${ZENITH_SRC_DIR}/Zenith/*.hpp
		${ZENITH_SRC_DIR}/Zenith/*.c
		${ZENITH_SRC_DIR}/Zenith/*.h
)

add_library(Zenith STATIC)

target_sources(Zenith PRIVATE ${ZENITH_SOURCES})

target_precompile_headers(Zenith PRIVATE ${ZENITH_SRC_DIR}/znpch.hpp)

target_compile_features(Zenith PUBLIC cxx_std_20)

if (MSVC)
	add_compile_options(/Gy)
	target_compile_options(Zenith PRIVATE /Zc:preprocessor)
endif()

target_include_directories(Zenith PUBLIC ${ZENITH_SRC_DIR})

# ==== Compile Definitions ====
target_compile_definitions(Zenith
	PUBLIC
		SDL_MAIN_HANDLED

		$<$<AND:$<BOOL:${ZENITH_TRACK_MEMORY}>,$<NOT:$<CONFIG:Release>>>:ZN_TRACK_MEMORY>
)

target_link_libraries(Zenith
	PUBLIC
		SDL3::SDL3-static
		spdlog
		glm
		glad
		Vulkan::Vulkan
		Tracy::TracyClient
		backward
)
