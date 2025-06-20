#include "znpch.hpp"

#include <unordered_set>
#include <atomic>

namespace Zenith {

	static std::unordered_set<void*> s_LiveReferences;
	static std::mutex s_LiveReferenceMutex;
	static std::atomic<bool> s_RefUtilsDestroyed{false};

	struct RefUtilsDestroyer {
		~RefUtilsDestroyer() {
			s_RefUtilsDestroyed.store(true);
		}
	};
	static RefUtilsDestroyer s_RefUtilsDestroyer;

	namespace RefUtils {

		void AddToLiveReferences(void* instance)
		{
			if (s_RefUtilsDestroyed.load())
				return;

			std::scoped_lock<std::mutex> lock(s_LiveReferenceMutex);
			ZN_CORE_ASSERT(instance);
			s_LiveReferences.insert(instance);
		}

		void RemoveFromLiveReferences(void* instance)
		{
			if (s_RefUtilsDestroyed.load())
				return;

			std::scoped_lock<std::mutex> lock(s_LiveReferenceMutex);
			ZN_CORE_ASSERT(instance);
			ZN_CORE_ASSERT(s_LiveReferences.find(instance) != s_LiveReferences.end());
			s_LiveReferences.erase(instance);
		}

		bool IsLive(void* instance)
		{
			if (s_RefUtilsDestroyed.load())
				return false;

			std::scoped_lock<std::mutex> lock(s_LiveReferenceMutex);
			ZN_CORE_ASSERT(instance);
			return s_LiveReferences.find(instance) != s_LiveReferences.end();
		}
	}

}