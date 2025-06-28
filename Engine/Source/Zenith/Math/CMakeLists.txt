set(MATH_SOURCES
		Math.cpp
)

set(MATH_HEADERS
		AABB.hpp
		Math.hpp
		Ray.hpp
)

target_sources(Zenith
		PRIVATE
		${MATH_SOURCES}
		${MATH_HEADERS}
)