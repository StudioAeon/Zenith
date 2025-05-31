#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <mutex>

#include "Log.hpp"

namespace Zenith {

	class Timer {
	public:
		Timer() { Reset(); }

		ZN_FORCE_INLINE void Reset() {
			m_Start = std::chrono::high_resolution_clock::now();
		}

		ZN_FORCE_INLINE float Elapsed() const {
			return std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - m_Start).count();
		}

		ZN_FORCE_INLINE float ElapsedMillis() const {
			return std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - m_Start).count();
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};

	class ScopedTimer {
	public:
		explicit ScopedTimer(std::string name)
			: m_Name(std::move(name)) {}

		~ScopedTimer() {
			float ms = m_Timer.ElapsedMillis();
			ZN_CORE_TRACE("{} - {:.3f}ms", m_Name, ms);
		}

	private:
		std::string m_Name;
		Timer m_Timer;
	};

	class PerformanceProfiler {
	public:
		struct PerFrameData {
			float Time = 0.0f;
			uint32_t Samples = 0;

			PerFrameData() = default;
			PerFrameData(float time) : Time(time), Samples(1) {}

			operator float() const { return Time; }

			PerFrameData& operator+=(float time) {
				Time += time;
				++Samples;
				return *this;
			}
		};

		void SetPerFrameTiming(const std::string& name, float time) {
			std::scoped_lock lock(m_PerFrameDataMutex);
			m_PerFrameData[name] += time;
		}

		void Clear() {
			std::scoped_lock lock(m_PerFrameDataMutex);
			m_PerFrameData.clear();
		}

		const std::unordered_map<std::string, PerFrameData>& GetPerFrameData() const {
			return m_PerFrameData;
		}

	private:
		std::unordered_map<std::string, PerFrameData> m_PerFrameData;
		inline static std::mutex m_PerFrameDataMutex;
	};

	class ScopePerfTimer {
	public:
		ScopePerfTimer(const char* name, PerformanceProfiler* profiler)
			: m_Name(name), m_Profiler(profiler) {}

		~ScopePerfTimer() {
			m_Profiler->SetPerFrameTiming(m_Name, m_Timer.ElapsedMillis());
		}

	private:
		std::string m_Name;
		PerformanceProfiler* m_Profiler;
		Timer m_Timer;
	};

#if 1
	#define ZN_SCOPE_PERF(name) \
		::Zenith::ScopePerfTimer CONCAT(scope_perf_, __LINE__)(name, ::Zenith::Application::Get().GetPerformanceProfiler())

	#define ZN_SCOPE_TIMER(name) \
		::Zenith::ScopedTimer CONCAT(scoped_timer_, __LINE__)(name)

	#define CONCAT_IMPL(x, y) x##y
	#define CONCAT(x, y) CONCAT_IMPL(x, y)
#else
	#define ZN_SCOPE_PERF(name)
	#define ZN_SCOPE_TIMER(name)
#endif

}