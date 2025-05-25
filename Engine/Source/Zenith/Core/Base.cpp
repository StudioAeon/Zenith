#include "znpch.hpp"
#include "Base.hpp"

#include "Log.hpp"

namespace Zenith {

	void InitializeCore()
	{
		Log::Init();

		ZN_CORE_TRACE("Zenith Engine {}", ZN_VERSION);
		ZN_CORE_TRACE("Intializing...");
	}

	void ShutdownCore()
	{
		ZN_CORE_TRACE("Shutting down...");
	}

}