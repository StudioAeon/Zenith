#include "znpch.hpp"
#include "Base.hpp"

#include "Log.hpp"
#include "Memory.hpp"

namespace Zenith {

	void InitializeCore()
	{
		Allocator::Init();
		Log::Init();

		ZN_CORE_TRACE_TAG("Core", "Zenith Engine {}", ZN_VERSION);
		ZN_CORE_TRACE_TAG("Core", "Intializing...");
	}

	void ShutdownCore()
	{
		ZN_CORE_TRACE_TAG("Core", "Shutting down...");
	}

}