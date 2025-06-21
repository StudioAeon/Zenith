# ==== STB (Header-only library) ====
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_LIST_DIR}/Assets/stb/include)

# ==== GLM (Header-only library) ====
set(GLM_BUILD_TESTS OFF)
set(GLM_BUILD_INSTALL OFF)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Rendering/glm SYSTEM)

# ==== SDL3 ====
set(SDL_AUDIO OFF)
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_EXAMPLES OFF)
set(SDL_TESTS OFF)
set(SDL_INSTALL OFF)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Core/SDL SYSTEM)

# ==== spdlog ====
set(SPDLOG_USE_STD_FORMAT ON)
set(SPDLOG_ENABLE_PCH ON)
set(SPDLOG_MSVC_UTF8 ON)
set(SPDLOG_BUILD_EXAMPLE OFF)
set(SPDLOG_BUILD_TESTS OFF)
set(SPDLOG_BUILD_BENCH OFF)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Core/spdlog SYSTEM)

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
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Rendering/tracy SYSTEM)

if(TARGET TracyClient AND ZENITH_TRACK_MEMORY)
	zenith_enable_lto(TracyClient)
endif()

# ==== VulkanMemoryAllocator ====
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Rendering/VulkanMemoryAllocator SYSTEM)

# ==== Backward-cpp (Stack traces) ====
set(BACKWARD_ENABLE TRUE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Utils/backward-cpp SYSTEM)

# ==== nlohmann/json ====
set(JSON_BuildTests OFF)
set(JSON_Install OFF)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Utils/json SYSTEM)

# ==== Native File Dialog Extended ====
set(NFD_BUILD_TESTS OFF)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Utils/nfd SYSTEM)

# ==== miniaudio (Header-only audio library) ====
add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE ${CMAKE_CURRENT_LIST_DIR}/Audio/miniaudio/include)

if(WIN32)
	target_compile_definitions(miniaudio INTERFACE
			MA_ENABLE_WASAPI=1
			MA_ENABLE_DSOUND=1
			MA_ENABLE_WINMM=1
	)
	target_link_libraries(miniaudio INTERFACE winmm ole32)
elseif(UNIX)
	target_compile_definitions(miniaudio INTERFACE
			MA_ENABLE_ALSA=1
			MA_ENABLE_PULSEAUDIO=1
			MA_ENABLE_JACK=1
	)
endif()

# ==== ImGui ====
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/UI/imgui)
file(GLOB IMGUI_SOURCES
		${IMGUI_DIR}/*.cpp
		${IMGUI_DIR}/*.h
		${IMGUI_DIR}/backends/imgui_impl_sdl3.*
		${IMGUI_DIR}/backends/imgui_impl_opengl3.*
		${IMGUI_DIR}/backends/imgui_impl_vulkan.*
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/UI
		${IMGUI_DIR}
)

target_link_libraries(imgui PUBLIC SDL3::SDL3-static Vulkan::Vulkan)

# ==== FileWatch ====
add_library(filewatch INTERFACE)
target_include_directories(filewatch INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}/Utils/filewatch/include
)
target_compile_features(filewatch INTERFACE cxx_std_20)

# ==== fast_obj (Header-only OBJ loader) ====
add_library(fast_obj INTERFACE)
target_include_directories(fast_obj INTERFACE ${CMAKE_CURRENT_LIST_DIR}/Assets/fast_obj)

# ==== fastgltf ====
set(FASTGLTF_COMPILE_AS_CPP20 ON)
set(FASTGLTF_ENABLE_TESTS OFF)
set(FASTGLTF_ENABLE_EXAMPLES OFF)
set(FASTGLTF_ENABLE_DOCS OFF)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Assets/fastgltf SYSTEM)

# ==== ufbx (Single-file FBX loader) ====
add_library(ufbx INTERFACE)
target_include_directories(ufbx INTERFACE ${CMAKE_CURRENT_LIST_DIR}/Assets/ufbx)

file(GLOB LUA_SOURCES
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lapi.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lcode.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lctype.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/ldebug.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/ldo.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/ldump.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lfunc.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lgc.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/llex.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lmem.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lobject.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lopcodes.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lparser.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lstate.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lstring.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/ltable.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/ltm.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lundump.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lvm.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lzio.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lauxlib.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lbaselib.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lcorolib.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/ldblib.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/liolib.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lmathlib.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/loslib.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lstrlib.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/ltablib.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/lutf8lib.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/loadlib.c
		${CMAKE_CURRENT_LIST_DIR}/Scripting/lua/linit.c
)

add_library(lua STATIC ${LUA_SOURCES})

target_include_directories(lua PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Scripting/lua)

target_compile_definitions(lua PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

set_target_properties(lua PROPERTIES
		LINKER_LANGUAGE CXX
)

add_library(lua::lua ALIAS lua)

# ==== Sol2 v3 (Header-Only) ====
add_library(sol2 INTERFACE)
target_include_directories(sol2 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/Scripting/sol2/include)
target_link_libraries(sol2 INTERFACE lua::lua)
target_compile_features(sol2 INTERFACE cxx_std_20)

add_library(sol2::sol2 ALIAS sol2)
