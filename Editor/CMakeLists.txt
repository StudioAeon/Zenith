set(EDITOR_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

file(GLOB_RECURSE ZENITH_EDITOR_SOURCES CONFIGURE_DEPENDS
		${EDITOR_SRC_DIR}/*.cpp
		${EDITOR_SRC_DIR}/*.hpp
		${EDITOR_SRC_DIR}/*.c
		${EDITOR_SRC_DIR}/*.h
)

list(JOIN ZENITH_EDITOR_SOURCES "\n    " ZENITH_EDITOR_PRINT)
message(STATUS "Zenith Editor sources:\n    ${ZENITH_EDITOR_PRINT}")

add_executable(Zenith-Editor ${ZENITH_EDITOR_SOURCES})

zenith_copy_resources(Zenith-Editor)

target_compile_features(Zenith-Editor PRIVATE cxx_std_20)

target_include_directories(Zenith-Editor PUBLIC ${EDITOR_SRC_DIR})

target_link_libraries(Zenith-Editor PRIVATE Zenith)

zenith_enable_lto(Zenith-Editor)