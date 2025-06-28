set(UI_SOURCES
		UILayer.cpp
)

set(UI_HEADERS
		UILayer.hpp
)

target_sources(Zenith
		PRIVATE
		${UI_SOURCES}
		${UI_HEADERS}
)