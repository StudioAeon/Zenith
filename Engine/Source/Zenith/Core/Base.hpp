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
	#define ZN_PLATFORM_UNIX
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	#define ZN_PLATFORM_BSD
	#define ZN_PLATFORM_UNIX
#elif defined(__unix__) || defined(__unix)
	#define ZN_PLATFORM_UNIX
#else
	#error "Unsupported platform! Zenith supports Windows, Linux, and BSD."
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

// Pointer wrappers
namespace Zenith {

	template<typename T>
	T RoundDown(T x, T fac) { return x / fac * fac; }

	template<typename T>
	T RoundUp(T x, T fac) { return RoundDown(x + fac - 1, fac); }

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	using byte = uint8_t;

}