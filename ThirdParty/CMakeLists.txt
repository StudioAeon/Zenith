# ==== STB (Header-only library) ====
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_LIST_DIR}/stb/include)

# ==== GLM (Header-only library) ====
set(GLM_BUILD_TESTS OFF)
set(GLM_BUILD_INSTALL OFF)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/glm SYSTEM)

# ==== SDL3 ====
set(SDL_AUDIO OFF)
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_EXAMPLES OFF)
set(SDL_TESTS OFF)
set(SDL_INSTALL OFF)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SDL SYSTEM)

# ==== spdlog ====
set(SPDLOG_USE_STD_FORMAT ON)
set(SPDLOG_ENABLE_PCH ON)
set(SPDLOG_MSVC_UTF8 ON)
set(SPDLOG_BUILD_EXAMPLE OFF)
set(SPDLOG_BUILD_TESTS OFF)
set(SPDLOG_BUILD_BENCH OFF)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/spdlog SYSTEM)

# ==== Tracy Profiler ====
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

if(TARGET TracyClient AND ZENITH_TRACK_MEMORY)
	zenith_enable_lto(TracyClient)
endif()

# ==== VulkanMemoryAllocator ====
add_subdirectory(VulkanMemoryAllocator SYSTEM)

# ==== Backward-cpp (Stack traces) ====
set(BACKWARD_ENABLE TRUE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/backward-cpp SYSTEM)

# ==== nlohmann/json ====
set(JSON_BuildTests OFF)
set(JSON_Install OFF)
add_subdirectory(json SYSTEM)

# ==== Native File Dialog Extended ====
set(NFD_BUILD_TESTS OFF)
add_subdirectory(nfd SYSTEM)

# ==== GLAD (OpenGL loader) ====
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/glad SYSTEM)

# ==== ImGui ====
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
file(GLOB IMGUI_SOURCES
		${IMGUI_DIR}/*.cpp
		${IMGUI_DIR}/*.h
		${IMGUI_DIR}/backends/imgui_impl_sdl3.*
		${IMGUI_DIR}/backends/imgui_impl_opengl3.*
		${IMGUI_DIR}/backends/imgui_impl_vulkan.*
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3-static Vulkan::Vulkan glad)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

# ==== FileWatch ====
add_library(filewatch INTERFACE)
target_include_directories(filewatch INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}/filewatch/include
)
target_compile_features(filewatch INTERFACE cxx_std_17)

