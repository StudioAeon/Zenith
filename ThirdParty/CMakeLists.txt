# ==== spdlog ====
set(SPDLOG_USE_STD_FORMAT ON)
set(SPDLOG_ENABLE_PCH ON)
set(SPDLOG_MSVC_UTF8 ON)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/spdlog SYSTEM)

# ==== SDL3 ====
set(SDL_AUDIO OFF)
set(SDL_EXAMPLES OFF)
set(SDL_TESTS OFF)
set(SDL_INSTALL OFF)
set(SDL_STATIC ON)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SDL SYSTEM)

# ==== GLM ====
set(GLM_BUILD_TESTS OFF)
set(GLM_BUILD_INSTALL OFF)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/glm SYSTEM)

# ==== GLAD ====
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/glad SYSTEM)

# ==== STB ====
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_LIST_DIR}/stb/include)

# ==== VulkanMemoryAllocator ====
set(VMA_STATIC_VULKAN_FUNCTIONS OFF)
set(VMA_DYNAMIC_VULKAN_FUNCTIONS ON)
add_subdirectory(VulkanMemoryAllocator)

# ==== nlohmann/json ====
set(JSON_BuildTests OFF)
set(JSON_Install OFF)
add_subdirectory(json)

# ==== Native File Dialog ====
set(NFD_BUILD_TESTS OFF)
add_subdirectory(nfd)

# ==== ImGui ====
set(IMGUI_DIR ${CMAKE_CURRENT_LIST_DIR}/imgui)

add_library(imgui STATIC)

target_sources(imgui PRIVATE
		${IMGUI_DIR}/imgui.cpp
		${IMGUI_DIR}/imgui_demo.cpp
		${IMGUI_DIR}/imgui_draw.cpp
		${IMGUI_DIR}/imgui_tables.cpp
		${IMGUI_DIR}/imgui_widgets.cpp

		${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp

		${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
		${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC
		${IMGUI_DIR}
		${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC
		SDL3::SDL3-static
)

target_link_libraries(imgui PUBLIC
		glad
		Vulkan::Vulkan
)

target_compile_definitions(imgui PUBLIC
		IMGUI_IMPL_OPENGL_LOADER_GLAD
		IMGUI_DISABLE_FREETYPE
)

target_compile_definitions(imgui PUBLIC
		$<$<CONFIG:Release>:IMGUI_DISABLE_DEMO_WINDOWS>
)

# ==== Tracy Profiler ====
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${IPO_SUPPORTED})

if(ZENITH_TRACK_MEMORY)
	set(TRACY_ENABLE ON)
	set(TRACY_ON_DEMAND OFF)
	set(TRACY_CALLSTACK ON)
	set(TRACY_ONLY_LOCALHOST ON)
	set(TRACY_NO_BROADCAST ON)
	set(TRACY_NO_VERIFY OFF)
	set(TRACY_SYMBOL_OFFLINE_RESOLVE OFF)
else()
	set(TRACY_ENABLE OFF)
endif()
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tracy SYSTEM)

# ==== Backward-cpp ====
	set(BACKWARD_ENABLE TRUE)
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/backward-cpp SYSTEM)

