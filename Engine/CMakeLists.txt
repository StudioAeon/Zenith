set(ZENITH_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

file(GLOB_RECURSE ZENITH_SOURCES CONFIGURE_DEPENDS
		${ZENITH_SRC_DIR}/Zenith/*.cpp
		${ZENITH_SRC_DIR}/Zenith/*.hpp
		${ZENITH_SRC_DIR}/Zenith/*.c
		${ZENITH_SRC_DIR}/Zenith/*.h
)

list(FILTER ZENITH_SOURCES EXCLUDE REGEX ".*/Platform/.*")

list(FILTER ZENITH_SOURCES EXCLUDE REGEX ".*/Platform/.*")

# Platform-specific source collection
if(WIN32)
	file(GLOB_RECURSE ZENITH_PLATFORM_SOURCES CONFIGURE_DEPENDS
			${ZENITH_SRC_DIR}/Zenith/Platform/Windows/*.cpp
			${ZENITH_SRC_DIR}/Zenith/Platform/Windows/*.hpp
			${ZENITH_SRC_DIR}/Zenith/Platform/Windows/*.c
			${ZENITH_SRC_DIR}/Zenith/Platform/Windows/*.h
	)
elseif(UNIX)
	file(GLOB_RECURSE ZENITH_PLATFORM_SOURCES CONFIGURE_DEPENDS
			${ZENITH_SRC_DIR}/Zenith/Platform/Unix/*.cpp
			${ZENITH_SRC_DIR}/Zenith/Platform/Unix/*.hpp
			${ZENITH_SRC_DIR}/Zenith/Platform/Unix/*.c
			${ZENITH_SRC_DIR}/Zenith/Platform/Unix/*.h
	)
else()
	message(FATAL_ERROR "Unsupported platform! Zenith Engine currently supports Windows and Unix-like systems only.")
endif()

list(APPEND ZENITH_SOURCES ${ZENITH_PLATFORM_SOURCES})

add_library(Zenith STATIC)

target_sources(Zenith PRIVATE ${ZENITH_SOURCES})

target_precompile_headers(Zenith PRIVATE ${ZENITH_SRC_DIR}/znpch.hpp)

target_compile_features(Zenith PUBLIC cxx_std_20)

target_include_directories(Zenith PUBLIC ${ZENITH_SRC_DIR})

# ==== Compile Definitions ====
target_compile_definitions(Zenith
	PUBLIC
		SDL_MAIN_HANDLED
		$<$<BOOL:>:ZN_TRACK_MEMORY>
)

target_link_libraries(Zenith
		PUBLIC
		SDL3::SDL3-static
		spdlog
		glm
		stb
		Vulkan::Vulkan
		VulkanMemoryAllocator
		Tracy::TracyClient
		backward
		nfd::nfd
		nlohmann_json::nlohmann_json
		imgui
		filewatch
		miniaudio
		fast_obj
		fastgltf::fastgltf
		ufbx
		lua::lua
		sol2::sol2
)

zenith_enable_lto(Zenith)
