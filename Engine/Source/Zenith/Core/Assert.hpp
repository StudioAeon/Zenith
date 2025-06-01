#pragma once

#include "Base.hpp"
#include "Log.hpp"

#ifdef ZN_PLATFORM_WINDOWS
	#define ZN_DEBUG_BREAK __debugbreak()
#elif defined(ZN_COMPILER_CLANG)
	#define ZN_DEBUG_BREAK __builtin_debugtrap()
#else
	#define ZN_DEBUG_BREAK
#endif

#ifdef ZN_DEBUG
	#define ZN_ENABLE_ASSERTS
#endif

#define ZN_ENABLE_VERIFY

#ifdef ZN_ENABLE_ASSERTS
	#ifdef ZN_COMPILER_CLANG
		#define ZN_CORE_ASSERT_MESSAGE_INTERNAL(...)  ::Zenith::Log::PrintAssertMessage(::Zenith::Log::Type::Core, "Assertion Failed", ##__VA_ARGS__)
		#define ZN_ASSERT_MESSAGE_INTERNAL(...)  ::Zenith::Log::PrintAssertMessage(::Zenith::Log::Type::Client, "Assertion Failed", ##__VA_ARGS__)
	#else
		#define ZN_CORE_ASSERT_MESSAGE_INTERNAL(...)  ::Zenith::Log::PrintAssertMessage(::Zenith::Log::Type::Core, "Assertion Failed" __VA_OPT__(,) __VA_ARGS__)
		#define ZN_ASSERT_MESSAGE_INTERNAL(...)  ::Zenith::Log::PrintAssertMessage(::Zenith::Log::Type::Client, "Assertion Failed" __VA_OPT__(,) __VA_ARGS__)
	#endif

	#define ZN_CORE_ASSERT(condition, ...) do { if(!(condition)) { ZN_CORE_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); ZN_DEBUG_BREAK; } } while(0)
	#define ZN_ASSERT(condition, ...) do { if(!(condition)) { ZN_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); ZN_DEBUG_BREAK; } } while(0)
#else
	#define ZN_CORE_ASSERT(condition, ...) ((void) (condition))
	#define ZN_ASSERT(condition, ...) ((void) (condition))
#endif

#ifdef ZN_ENABLE_VERIFY
	#ifdef ZN_COMPILER_CLANG
		#define ZN_CORE_VERIFY_MESSAGE_INTERNAL(...)  ::Zenith::Log::PrintAssertMessage(::Zenith::Log::Type::Core, "Verify Failed", ##__VA_ARGS__)
		#define ZN_VERIFY_MESSAGE_INTERNAL(...)  ::Zenith::Log::PrintAssertMessage(::Zenith::Log::Type::Client, "Verify Failed", ##__VA_ARGS__)
	#else
		#define ZN_CORE_VERIFY_MESSAGE_INTERNAL(...)  ::Zenith::Log::PrintAssertMessage(::Zenith::Log::Type::Core, "Verify Failed" __VA_OPT__(,) __VA_ARGS__)
		#define ZN_VERIFY_MESSAGE_INTERNAL(...)  ::Zenith::Log::PrintAssertMessage(::Zenith::Log::Type::Client, "Verify Failed" __VA_OPT__(,) __VA_ARGS__)
	#endif

	#define ZN_CORE_VERIFY(condition, ...) do { if(!(condition)) { ZN_CORE_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); ZN_DEBUG_BREAK; } } while(0)
	#define ZN_VERIFY(condition, ...) do { if(!(condition)) { ZN_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); ZN_DEBUG_BREAK; } } while(0)
#else
	#define ZN_CORE_VERIFY(condition, ...) ((void) (condition))
	#define ZN_VERIFY(condition, ...) ((void) (condition))
#endif