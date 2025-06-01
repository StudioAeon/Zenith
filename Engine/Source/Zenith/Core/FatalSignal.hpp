#pragma once

#include <functional>

namespace Zenith {

	class FatalSignal {
	public:
		using Callback = std::function<void()>;

		static void Install(long timeoutMs = 2000);

		static void AddCallback(const Callback& callback);

		static void Die();

	private:
		static FatalSignal s_Instance;

		std::vector<Callback> m_Callbacks;
		long m_TimeoutMs = 0;
		bool m_IsActive = false;

		void Handle(const char* reason);

		static void OnTimeout();
		static void OnTerminate();
	};
}