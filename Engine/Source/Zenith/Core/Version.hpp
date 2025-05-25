#pragma once

#define ZN_VERSION "v0.1.0"

//
// Build Configuration
//
#if defined(ZN_DEBUG)
#define ZN_BUILD_CONFIG_NAME "Debug"
#elif defined(ZN_RELEASE)
#define ZN_BUILD_CONFIG_NAME "Release"
#elif defined(ZN_DIST)
#define ZN_BUILD_CONFIG_NAME "Dist"
#else
#error Undefined configuration?
#endif

//
// Build Platform
//
#if defined(ZN_PLATFORM_WINDOWS)
#define ZN_BUILD_PLATFORM_NAME "Windows x64"
#elif defined(ZN_PLATFORM_LINUX)
#define ZN_BUILD_PLATFORM_NAME "Linux"
#else
#define ZN_BUILD_PLATFORM_NAME "Unknown"
#endif

#define ZN_VERSION_LONG "Zenith " ZN_VERSION " (" ZN_PLATOFRM_NAME " " ZN_BUILD_CONFIG_NAME ")"

// Stable build version (YEAR.SEASON.MAJOR.MINOR) Season is 1(Winter), 2(Spring), 3(Summer), 4(Fall)