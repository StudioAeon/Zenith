#pragma once

#define ZN_ENABLE_PROFILING !ZN_DIST

#if ZN_ENABLE_PROFILING
#include <tracy/Tracy.hpp>
#endif

#if ZN_ENABLE_PROFILING
	#define ZN_PROFILE_MARK_FRAME			FrameMark;
	#define ZN_PROFILE_FUNC(...)			ZoneScoped##__VA_OPT__(N(__VA_ARGS__))
	#define ZN_PROFILE_SCOPE(...)			ZN_PROFILE_FUNC(__VA_ARGS__)
	#define ZN_PROFILE_SCOPE_DYNAMIC(NAME)  ZoneScoped; ZoneName(NAME, strlen(NAME))
	#define ZN_PROFILE_THREAD(...)          tracy::SetThreadName(__VA_ARGS__)
#else
	#define ZN_PROFILE_MARK_FRAME
	#define ZN_PROFILE_FUNC(...)
	#define ZN_PROFILE_SCOPE(...)
	#define ZN_PROFILE_SCOPE_DYNAMIC(NAME)
	#define ZN_PROFILE_THREAD(...)
#endif