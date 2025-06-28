set(REFLECTION_HEADERS
		MetaHelpers.hpp
		TypeDescriptor.hpp
		TypeName.hpp
		TypeStructures.hpp
		TypeUtils.hpp
)

target_sources(Zenith
		PRIVATE
		${REFLECTION_SOURCES}
		${REFLECTION_HEADERS}
)