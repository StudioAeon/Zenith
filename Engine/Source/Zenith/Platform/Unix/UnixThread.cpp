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

}
