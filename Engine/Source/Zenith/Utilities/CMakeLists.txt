set(UTILITIES_SOURCES
		CommandLineParser.cpp
		FileSystem.cpp
		StringUtils.cpp
)

set(UTILITIES_HEADERS
		CommandLineParser.hpp
		ContainerUtils.hpp
		DurationUtils.hpp
		FileSystem.hpp
		JSONSerializationHelpers.hpp
		ProcessHelper.hpp
		SerializationMacros.hpp
		StringUtils.hpp
		TypeInfo.hpp
)

target_sources(Zenith
		PRIVATE
		${UTILITIES_SOURCES}
		${UTILITIES_HEADERS}
)