#pragma once

#include <functional>
#include <memory>

namespace Zenith
{
	void InitializeCore();
	void ShutdownCore();
};

#if defined(_WIN64) || defined(_WIN32)
		#define ZN_PLATFORM_WINDOWS
#elif defined(__linux__)
		#define ZN_PLATFORM_LINUX
#else
		#error "Unsupported platform! Zenith only supports Windows and Linux."
#endif

#define BIT(x) (1u << x)

//------------------------------------------------------------------------------
// Compiler Detection
//------------------------------------------------------------------------------

#if defined(__clang__)
	#define ZN_COMPILER_CLANG
#elif defined(__GNUC__)
	#define ZN_COMPILER_GCC
#elif defined(_MSC_VER)
	#define ZN_COMPILER_MSVC
#else
	#error "Unknown compiler! Zenith only supports MSVC, GCC, and Clang."
#endif

//------------------------------------------------------------------------------
// Function Inlining & Static Declaration
//------------------------------------------------------------------------------

#if defined(ZN_COMPILER_MSVC)
	#define ZN_FORCE_INLINE    __forceinline
	#define ZN_EXPLICIT_STATIC static
#elif defined(ZN_COMPILER_GCC) || defined(ZN_COMPILER_CLANG)
	#define ZN_FORCE_INLINE    __attribute__((always_inline)) inline
	#define ZN_EXPLICIT_STATIC
#else
	#define ZN_FORCE_INLINE    inline
	#define ZN_EXPLICIT_STATIC
#endif