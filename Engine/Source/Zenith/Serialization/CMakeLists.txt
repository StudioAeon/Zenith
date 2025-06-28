set(SERIALIZATION_SOURCES
		FileStream.cpp
		MemoryStream.cpp
		StreamReader.cpp
		StreamWriter.cpp
)

set(SERIALIZATION_HEADERS
		FileStream.hpp
		MemoryStream.hpp
		Serialization.hpp
		SerializationImpl.hpp
		StreamReader.hpp
		StreamWriter.hpp
)

target_sources(Zenith
		PRIVATE
		${SERIALIZATION_SOURCES}
		${SERIALIZATION_HEADERS}
)