#include "znpch.hpp"
#include "FatalSignal.hpp"

#ifndef ZN_PLATFORM_WINDOWS
#include <unistd.h>
#endif

#include <backward.hpp>

namespace Zenith
{
	FatalSignal FatalSignal::s_Instance;

	void FatalSignal::Die()
	{
		backward::StackTrace st;
		st.load_here(32);
		backward::Printer().print(st);

		_Exit(-1); // Immediately terminate without cleanup
	}

	void FatalSignal::OnTimeout()
	{
		puts("FATAL SIGNAL TIMEOUT");
		Die();
	}

	void FatalSignal::Handle(const char* reason)
	{
		if (m_IsActive)
		{
			puts("NESTED FATAL ERROR");
			puts(reason);
			Die();
		}

		puts("FATAL SIGNAL RECEIVED");
		puts(reason);

		m_IsActive = true;

#ifndef ZN_PLATFORM_WINDOWS
		ualarm(static_cast<useconds_t>(m_TimeoutMs * 1000), 0);
#else
		std::thread([this]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(m_TimeoutMs));
			OnTimeout();
		}).detach();
#endif

		for (auto& cb : m_Callbacks)
			cb();

		Die();
	}

	void FatalSignal::Install(long timeoutMs)
	{
		s_Instance.m_TimeoutMs = timeoutMs;

		std::set_terminate([] {
			const char* message = "<unknown>";
			try {
				auto eptr = std::current_exception();
				if (eptr) std::rethrow_exception(eptr);
			}
			catch (const std::exception& e) {
				message = e.what();
			}
			catch (...) {
				message = "<non-standard exception>";
			}
			s_Instance.Handle(message);
		});

		auto signalHandler = [](int sig) {
			const char* signalName = "<unknown>";
			switch (sig) {
				case SIGABRT: signalName = "SIGABRT"; break;
				case SIGFPE:  signalName = "SIGFPE";  break;
				case SIGILL:  signalName = "SIGILL";  break;
				case SIGINT:  signalName = "SIGINT";  break;
				case SIGSEGV: signalName = "SIGSEGV"; break;
				case SIGTERM: signalName = "SIGTERM"; break;
			}
			s_Instance.Handle(signalName);
		};

		signal(SIGABRT, signalHandler);
		signal(SIGFPE,  signalHandler);
		signal(SIGILL,  signalHandler);
		signal(SIGINT,  signalHandler);
		signal(SIGSEGV, signalHandler);
		signal(SIGTERM, signalHandler);

#ifndef ZN_PLATFORM_WINDOWS
		signal(SIGALRM, [](int) { s_Instance.OnTimeout(); });
#endif
	}

	void FatalSignal::AddCallback(const Callback& callback)
	{
		s_Instance.m_Callbacks.emplace_back(callback);
	}
}