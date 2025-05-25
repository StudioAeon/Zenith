#pragma once

#include "Zenith/Core/Base.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

#define ZN_ENABLE_LOGGING
namespace Zenith {

	class Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}

#ifdef ZN_ENABLE_LOGGING

	// Core
	#define ZN_CORE_TRACE(...)   ::Zenith::Log::GetCoreLogger()->trace(__VA_ARGS__)
	#define ZN_CORE_INFO(...)    ::Zenith::Log::GetCoreLogger()->info(__VA_ARGS__)
	#define ZN_CORE_WARN(...)    ::Zenith::Log::GetCoreLogger()->warn(__VA_ARGS__)
	#define ZN_CORE_ERROR(...)   ::Zenith::Log::GetCoreLogger()->error(__VA_ARGS__)
	#define ZN_CORE_FATAL(...)   ::Zenith::Log::GetCoreLogger()->critical(__VA_ARGS__)
	
	// Client
	#define ZN_TRACE(...)        ::Zenith::Log::GetClientLogger()->trace(__VA_ARGS__)
	#define ZN_INFO(...)         ::Zenith::Log::GetClientLogger()->info(__VA_ARGS__)
	#define ZN_WARN(...)         ::Zenith::Log::GetClientLogger()->warn(__VA_ARGS__)
	#define ZN_ERROR(...)        ::Zenith::Log::GetClientLogger()->error(__VA_ARGS__)
	#define ZN_FATAL(...)        ::Zenith::Log::GetClientLogger()->critical(__VA_ARGS__)

#else

	// Logging disabled: no-ops
	#define ZN_CORE_TRACE(...)
	#define ZN_CORE_INFO(...)
	#define ZN_CORE_WARN(...)
	#define ZN_CORE_ERROR(...)
	#define ZN_CORE_FATAL(...)
	
	#define ZN_TRACE(...)
	#define ZN_INFO(...)
	#define ZN_WARN(...)
	#define ZN_ERROR(...)
	#define ZN_FATAL(...)

#endif