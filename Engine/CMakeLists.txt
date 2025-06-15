set(ZENITH_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

file(GLOB_RECURSE ZENITH_SOURCES CONFIGURE_DEPENDS
		${ZENITH_SRC_DIR}/Zenith/*.cpp
		${ZENITH_SRC_DIR}/Zenith/*.hpp
		${ZENITH_SRC_DIR}/Zenith/*.c
		${ZENITH_SRC_DIR}/Zenith/*.h
)

list(FILTER ZENITH_SOURCES EXCLUDE REGEX ".*/Platform/.*")

set(ZENITH_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

# Collect all sources EXCEPT platform-specific directories
file(GLOB_RECURSE ZENITH_SOURCES CONFIGURE_DEPENDS
		${ZENITH_SRC_DIR}/Zenith/*.cpp
		${ZENITH_SRC_DIR}/Zenith/*.hpp
		${ZENITH_SRC_DIR}/Zenith/*.c
		${ZENITH_SRC_DIR}/Zenith/*.h
)

# Remove platform-specific files from the main collection
list(FILTER ZENITH_SOURCES EXCLUDE REGEX ".*/Platform/.*")

set(ZENITH_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

# Collect all sources EXCEPT platform-specific directories
file(GLOB_RECURSE ZENITH_SOURCES CONFIGURE_DEPENDS
		${ZENITH_SRC_DIR}/Zenith/*.cpp
		${ZENITH_SRC_DIR}/Zenith/*.hpp
		${ZENITH_SRC_DIR}/Zenith/*.c
		${ZENITH_SRC_DIR}/Zenith/*.h
)

list(FILTER ZENITH_SOURCES EXCLUDE REGEX ".*/Platform/.*")

if(WIN32)
	file(GLOB_RECURSE ZENITH_PLATFORM_SOURCES CONFIGURE_DEPENDS
			${ZENITH_SRC_DIR}/Zenith/Platform/Windows/*.cpp
			${ZENITH_SRC_DIR}/Zenith/Platform/Windows/*.hpp
			${ZENITH_SRC_DIR}/Zenith/Platform/Windows/*.c
			${ZENITH_SRC_DIR}/Zenith/Platform/Windows/*.h
	)
	list(APPEND ZENITH_SOURCES ${ZENITH_PLATFORM_SOURCES})
elseif(UNIX)
	file(GLOB_RECURSE ZENITH_PLATFORM_SOURCES CONFIGURE_DEPENDS
			${ZENITH_SRC_DIR}/Zenith/Platform/Linux/*.cpp
			${ZENITH_SRC_DIR}/Zenith/Platform/Linux/*.hpp
			${ZENITH_SRC_DIR}/Zenith/Platform/Linux/*.c
			${ZENITH_SRC_DIR}/Zenith/Platform/Linux/*.h
	)
	list(APPEND ZENITH_SOURCES ${ZENITH_PLATFORM_SOURCES})
else()
	message(FATAL_ERROR "Unsupported platform! Zenith Engine currently supports Windows and Linux only.")
endif()

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
		glad
		stb
		Vulkan::Vulkan
		VulkanMemoryAllocator
		Tracy::TracyClient
		backward
		nfd::nfd
		nlohmann_json::nlohmann_json
		imgui
		filewatch
)
