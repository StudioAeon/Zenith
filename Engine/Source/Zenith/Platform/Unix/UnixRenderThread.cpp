#include "znpch.hpp"
#include "Zenith/Renderer/RenderThread.hpp"

#include "Zenith/Renderer/Renderer.hpp"

#include <condition_variable>
#include <mutex>

namespace Zenith {

	struct RenderThreadData
	{
		std::mutex m_CriticalSection;
		std::condition_variable m_ConditionVariable;

		RenderThread::State m_State = RenderThread::State::Idle;
	};

	static std::thread::id s_RenderThreadID;

	RenderThread::RenderThread(ThreadingPolicy coreThreadingPolicy)
		: m_RenderThread("Render Thread"), m_ThreadingPolicy(coreThreadingPolicy)
	{
		m_Data = new RenderThreadData();
	}

	RenderThread::~RenderThread()
	{
		s_RenderThreadID = std::thread::id();
	}

	void RenderThread::Run()
	{
		m_IsRunning = true;
		if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
			// m_RenderThread.Dispatch(Renderer::RenderThreadFunc, this);

		s_RenderThreadID = m_RenderThread.GetID();
	}

	void RenderThread::Terminate()
	{
		m_IsRunning = false;
		Pump();

		if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
			m_RenderThread.Join();

		s_RenderThreadID = std::thread::id();
	}

	void RenderThread::Wait(State waitForState)
	{
		if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
			return;

		std::unique_lock lock(m_Data->m_CriticalSection);
		while (m_Data->m_State != waitForState)
		{
			m_Data->m_ConditionVariable.wait(lock);
		}
	}

	void RenderThread::WaitAndSet(State waitForState, State setToState)
	{
		if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
			return;

		std::unique_lock lock(m_Data->m_CriticalSection);
		while (m_Data->m_State != waitForState)
		{
			m_Data->m_ConditionVariable.wait(lock);
		}
		m_Data->m_State = setToState;
		m_Data->m_ConditionVariable.notify_one();
	}

	void RenderThread::Set(State setToState)
	{
		if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
			return;

		std::unique_lock lock(m_Data->m_CriticalSection);
		m_Data->m_State = setToState;
		m_Data->m_ConditionVariable.notify_one();
	}

	void RenderThread::NextFrame()
	{
		m_AppThreadFrame++;
		Renderer::SwapQueues();
	}

	void RenderThread::BlockUntilRenderComplete()
	{
		if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
			return;

		Wait(State::Idle);
	}

	void RenderThread::Kick()
	{
		if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
		{
			Set(State::Kick);
		}
		else
		{
			Renderer::WaitAndRender(this);
		}
	}

	void RenderThread::Pump()
	{
		NextFrame();
		Kick();
		BlockUntilRenderComplete();
	}

	bool RenderThread::IsCurrentThreadRT()
	{
		// NOTE: for debugging
		// ZN_CORE_VERIFY(s_RenderThreadID != std::thread::id());
		return s_RenderThreadID == std::this_thread::get_id();
	}
}