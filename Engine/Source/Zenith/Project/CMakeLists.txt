set(PROJECT_SOURCES
		Project.cpp
		ProjectSerializer.cpp
		UserPreferences.cpp
)

set(PROJECT_HEADERS
		Project.hpp
		ProjectSerializer.hpp
		UserPreferences.hpp
)

target_sources(Zenith
		PRIVATE
		${PROJECT_SOURCES}
		${PROJECT_HEADERS}
)