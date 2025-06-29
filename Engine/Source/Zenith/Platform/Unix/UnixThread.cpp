#include "znpch.hpp"
#include "Zenith/Core/Thread.hpp"

#include <pthread.h>
#include <chrono>

namespace Zenith {

	struct ThreadSignalData {
		pthread_mutex_t mutex;
		pthread_cond_t cond;
		bool signaled = false;
		bool manualReset = false;
	};

	Thread::Thread(const std::string& name)
		: m_Name(name)
	{
	}

	ThreadSignal::ThreadSignal(const std::string& name, bool manualReset)
	{
		(void)name; // optional
		auto data = new ThreadSignalData();
		pthread_mutex_init(&data->mutex, nullptr);
		pthread_cond_init(&data->cond, nullptr);
		data->manualReset = manualReset;
		data->signaled = false;
		m_SignalHandle = data;
	}

	void Thread::SetName(const std::string& name)
	{
		pthread_setname_np(m_Thread.native_handle(), name.substr(0, 15).c_str());
	}

	void Thread::Join()
	{
		if (m_Thread.joinable())
			m_Thread.join();
	}

	ThreadSignal::~ThreadSignal()
	{
		auto data = static_cast<ThreadSignalData*>(m_SignalHandle);
		if (data) {
			pthread_cond_destroy(&data->cond);
			pthread_mutex_destroy(&data->mutex);
			delete data;
			m_SignalHandle = nullptr;
		}
	}

	void ThreadSignal::Wait()
	{
		auto data = static_cast<ThreadSignalData*>(m_SignalHandle);
		pthread_mutex_lock(&data->mutex);
		while (!data->signaled)
			pthread_cond_wait(&data->cond, &data->mutex);
		if (!data->manualReset)
			data->signaled = false;
		pthread_mutex_unlock(&data->mutex);
	}

	void ThreadSignal::Signal()
	{
		auto data = static_cast<ThreadSignalData*>(m_SignalHandle);
		pthread_mutex_lock(&data->mutex);
		data->signaled = true;
		if (data->manualReset)
			pthread_cond_broadcast(&data->cond);
		else
			pthread_cond_signal(&data->cond);
		pthread_mutex_unlock(&data->mutex);
	}

	void ThreadSignal::Reset()
	{
		auto data = static_cast<ThreadSignalData*>(m_SignalHandle);
		pthread_mutex_lock(&data->mutex);
		data->signaled = false;
		pthread_mutex_unlock(&data->mutex);
	}

	std::thread::id Thread::GetID() const
	{
		return m_Thread.get_id();
	}

}
