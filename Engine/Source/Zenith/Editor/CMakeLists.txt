set(EDITOR_SOURCES
		EditorCamera.cpp
)

set(EDITOR_HEADERS
		EditorCamera.hpp
)

target_sources(Zenith
		PRIVATE
		${EDITOR_SOURCES}
		${EDITOR_HEADERS}
)