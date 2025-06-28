set(EVENTS_HEADERS
		ApplicationEvent.hpp
		EditorEvent.hpp
		Event.hpp
		KeyEvent.hpp
		MouseEvent.hpp
)

target_sources(Zenith
		PRIVATE
		${EVENTS_SOURCES}
		${EVENTS_HEADERS}
)