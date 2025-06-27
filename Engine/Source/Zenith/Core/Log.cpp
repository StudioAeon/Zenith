#include "znpch.hpp"
#include "Log.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <filesystem>

#define ZN_HAS_CONSOLE !ZN_DIST

namespace Zenith {

	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	std::map<std::string, Log::TagDetails> Log::s_DefaultTagDetails = {
		{ "Core",              TagDetails{  true, Level::Trace } },
		{ "AssetManager",      TagDetails{  true, Level::Info  } },
		{ "AssetSystem",       TagDetails{  true, Level::Info  } },
		{ "SDL",               TagDetails{  true, Level::Error } },
		{ "Mesh",              TagDetails{  true, Level::Warn  } },
		{ "Project",           TagDetails{  true, Level::Warn  } },
		{ "Memory",            TagDetails{  true, Level::Error } },
		{ "Scene",             TagDetails{  true, Level::Info  } },
		{ "Renderer",          TagDetails{  true, Level::Info  } },
		{ "Timer",             TagDetails{ false, Level::Trace } },
	};

	void Log::Init()
	{
		std::string logsDirectory = "logs";
		if (!std::filesystem::exists(logsDirectory))
			std::filesystem::create_directories(logsDirectory);

		std::vector<spdlog::sink_ptr> zenithSinks =
		{
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/ZENITH.log", true),
#if ZN_HAS_CONSOLE
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
#endif
		};

		std::vector<spdlog::sink_ptr> appSinks =
		{
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/APP.log", true),
#if ZN_HAS_CONSOLE
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
#endif
		};

		zenithSinks[0]->set_pattern("[%T] [%l] %n: %v");
		appSinks[0]->set_pattern("[%T] [%l] %n: %v");

#if ZN_HAS_CONSOLE
		zenithSinks[1]->set_pattern("%^[%T] %n: %v%$");
		appSinks[1]->set_pattern("%^[%T] %n: %v%$");
#endif

		s_CoreLogger = std::make_shared<spdlog::logger>("ZENITH", zenithSinks.begin(), zenithSinks.end());
		s_CoreLogger->set_level(spdlog::level::trace);

		s_ClientLogger = std::make_shared<spdlog::logger>("APP", appSinks.begin(), appSinks.end());
		s_ClientLogger->set_level(spdlog::level::trace);

		SetDefaultTagSettings();
	}

	void Log::Shutdown()
	{
		s_ClientLogger.reset();
		s_CoreLogger.reset();
		spdlog::drop_all();
	}

	void Log::SetDefaultTagSettings()
	{
		s_EnabledTags = s_DefaultTagDetails;
	}

}
