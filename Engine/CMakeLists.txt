set(ZENITH_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

# ==== Root-level infrastructure files ====
set(ZENITH_ROOT_SOURCES
		${ZENITH_SRC_DIR}/znpch.cpp
)

set(ZENITH_ROOT_HEADERS
		${ZENITH_SRC_DIR}/Zenith.hpp
		${ZENITH_SRC_DIR}/Zenith/EntryPoint.hpp
)

# ==== Third-party implementation files ====
set(ZENITH_THIRDPARTY_IMPL_SOURCES
		${ZENITH_SRC_DIR}/Zenith/ThirdParty/stb_impl.cpp
		${ZENITH_SRC_DIR}/Zenith/ThirdParty/VMAImplementation.cpp
		${ZENITH_SRC_DIR}/Zenith/ThirdParty/miniaudio_impl.cpp
		${ZENITH_SRC_DIR}/Zenith/ThirdParty/modelloading_impl.cpp
)

add_library(Zenith STATIC)

target_sources(Zenith PRIVATE ${ZENITH_ROOT_SOURCES} ${ZENITH_ROOT_HEADERS} ${ZENITH_THIRDPARTY_IMPL_SOURCES})

add_subdirectory(${ZENITH_SRC_DIR}/Zenith/Asset)
add_subdirectory(${ZENITH_SRC_DIR}/Zenith/Core)
add_subdirectory(${ZENITH_SRC_DIR}/Zenith/Debug)
add_subdirectory(${ZENITH_SRC_DIR}/Zenith/Editor)
add_subdirectory(${ZENITH_SRC_DIR}/Zenith/Events)
add_subdirectory(${ZENITH_SRC_DIR}/Zenith/ImGui)
add_subdirectory(${ZENITH_SRC_DIR}/Zenith/Math)
add_subdirectory(${ZENITH_SRC_DIR}/Zenith/Platform)
add_subdirectory(${ZENITH_SRC_DIR}/Zenith/Project)
add_subdirectory(${ZENITH_SRC_DIR}/Zenith/Reflection)
add_subdirectory(${ZENITH_SRC_DIR}/Zenith/Renderer)
add_subdirectory(${ZENITH_SRC_DIR}/Zenith/Serialization)
add_subdirectory(${ZENITH_SRC_DIR}/Zenith/UI)
add_subdirectory(${ZENITH_SRC_DIR}/Zenith/Utilities)

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
		# ==== Core Dependencies ====
		SDL3::SDL3-static
		spdlog
		glm
		stb

		# ==== Graphics & Rendering ====
		Vulkan::Vulkan
		VulkanMemoryAllocator
		imgui
		Tracy::TracyClient
		backward

		# ==== UI, File System, Events ====
		filewatch
		nfd::nfd
		nlohmann_json::nlohmann_json

		# ==== Audio ====
		miniaudio

		# ==== Asset Loading ====
		fast_obj
		fastgltf::fastgltf
		ufbx

		# ==== Scripting ====
		lua::lua
		sol2::sol2

		dxcompiler
		zenith-reflect
)

zenith_enable_lto(Zenith)
