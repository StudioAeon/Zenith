if(WIN32)
	target_sources(Zenith
			PRIVATE
			Windows/WindowsFileSystem.cpp
			Windows/WindowsProcessHelper.cpp
			Windows/WindowsRenderThread.cpp
			Windows/WindowsThread.cpp
	)

	target_compile_definitions(Zenith PRIVATE ZN_PLATFORM_WINDOWS)

elseif(UNIX)
	target_sources(Zenith
			PRIVATE
			Unix/UnixFileSystem.cpp
			Unix/UnixProcessHelper.cpp
			Unix/UnixRenderThread.cpp
			Unix/UnixThread.cpp
	)

	target_compile_definitions(Zenith PRIVATE ZN_PLATFORM_UNIX)

else()
	message(FATAL_ERROR "Unsupported platform! Zenith Engine currently supports Windows and Unix-like systems only.")
endif()