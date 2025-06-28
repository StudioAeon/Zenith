set(EDITOR_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

set(EDITOR_SOURCES
		${EDITOR_SRC_DIR}/EditorLayer.cpp
		${EDITOR_SRC_DIR}/Zenith-Editor.cpp
)

set(EDITOR_HEADERS
		${EDITOR_SRC_DIR}/EditorLayer.hpp
)

add_executable(Zenith-Editor ${EDITOR_SOURCES} ${EDITOR_HEADERS})

zenith_copy_directory(Zenith-Editor Editor/Resources Resources)
zenith_copy_directory(Zenith-Editor Editor/ProjectApex ProjectApex)

if(DXC_BINARY_DIR)
	zenith_copy_directory_absolute(Zenith-Editor "${DXC_BINARY_DIR}" ".")
endif()

target_compile_features(Zenith-Editor PRIVATE cxx_std_20)
target_include_directories(Zenith-Editor PUBLIC ${EDITOR_SRC_DIR})

target_link_libraries(Zenith-Editor PRIVATE Zenith)

zenith_enable_lto(Zenith-Editor)