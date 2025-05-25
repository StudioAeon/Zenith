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
#define ZN_EXPAND_VARGS(x) x
#endif

#ifdef ZN_ENABLE_ASSERTS
#define ZN_ASSERT_NO_MESSAGE(condition) { if(!(condition)) { ZN_ERROR("Assertion Failed"); __debugbreak(); } }
#define ZN_ASSERT_MESSAGE(condition, ...) { if(!(condition)) { ZN_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }

#define ZN_ASSERT_RESOLVE(arg1, arg2, macro, ...) macro
#define ZN_GET_ASSERT_MACRO(...) ZN_EXPAND_VARGS(ZN_ASSERT_RESOLVE(__VA_ARGS__, ZN_ASSERT_MESSAGE, ZN_ASSERT_NO_MESSAGE))

#define ZN_ASSERT(...) ZN_EXPAND_VARGS( ZN_GET_ASSERT_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#define ZN_CORE_ASSERT(...) ZN_EXPAND_VARGS( ZN_GET_ASSERT_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#else
#define ZN_ASSERT(...)
#define ZN_CORE_ASSERT(...)
#endif