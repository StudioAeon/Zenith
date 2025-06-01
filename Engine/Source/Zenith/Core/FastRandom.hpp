#pragma once

#include <cstdint>
#include <chrono>
#include <cmath>
#include <limits>
#include <type_traits>

namespace Zenith {

	// High-performance PCG32 random number generator
	class FastRandom
	{
	public:
		using result_type = uint32_t;
		static constexpr result_type min() { return 0; }
		static constexpr result_type max() { return UINT32_MAX; }

		FastRandom() noexcept : m_State(DEFAULT_SEED), m_Inc(DEFAULT_INC) {}
		
		explicit FastRandom(uint64_t seed) noexcept 
		{
			SetSeed(seed);
		}

		FastRandom(uint64_t seed, uint64_t sequence) noexcept
		{
			SetSeed(seed, sequence);
		}

		void SetSeed(uint64_t seed) noexcept
		{
			SetSeed(seed, DEFAULT_INC);
		}

		void SetSeed(uint64_t seed, uint64_t sequence) noexcept
		{
			m_State = 0U;
			m_Inc = (sequence << 1u) | 1u;
			NextUInt32();
			m_State += seed;
			NextUInt32();
		}

		uint64_t GetCurrentSeed() const noexcept { return m_State; }

		uint32_t operator()() noexcept { return NextUInt32(); }

		uint32_t NextUInt32() noexcept
		{
			uint64_t oldstate = m_State;
			m_State = oldstate * PCG_MULTIPLIER + m_Inc;
			uint32_t xorshifted = static_cast<uint32_t>(((oldstate >> 18u) ^ oldstate) >> 27u);
			uint32_t rot = static_cast<uint32_t>(oldstate >> 59u);
			return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
		}

		int32_t NextInt32() noexcept
		{
			return static_cast<int32_t>(NextUInt32());
		}

		uint16_t NextUInt16() noexcept
		{
			return static_cast<uint16_t>(NextUInt32() >> 16);
		}

		int16_t NextInt16() noexcept
		{
			return static_cast<int16_t>(NextUInt16());
		}

		uint8_t NextUInt8() noexcept
		{
			return static_cast<uint8_t>(NextUInt32() >> 24);
		}

		int8_t NextInt8() noexcept
		{
			return static_cast<int8_t>(NextUInt8());
		}

		float NextFloat() noexcept
		{
			return static_cast<float>(NextUInt32() >> 8) * FLOAT_DIVISOR;
		}

		double NextDouble() noexcept
		{
			uint64_t high = static_cast<uint64_t>(NextUInt32()) << 21;
			uint64_t low = NextUInt32() >> 11;
			return static_cast<double>(high | low) * DOUBLE_DIVISOR;
		}

		template<typename T>
		T NextInRange(T min_val, T max_val) noexcept
		{
			static_assert(std::is_arithmetic_v<T>, "T must be arithmetic type");

			if constexpr (std::is_integral_v<T>)
			{
				return NextIntInRange(min_val, max_val);
			}
			else
			{
				return NextFloatInRange(min_val, max_val);
			}
		}

		int32_t NextIntInRange(int32_t min_val, int32_t max_val) noexcept
		{
			if (min_val >= max_val) return min_val;
			
			uint32_t range = static_cast<uint32_t>(max_val - min_val);
			uint32_t threshold = (UINT32_MAX - range + 1) % (range + 1);

			uint32_t result;
			do {
				result = NextUInt32();
			} while (result < threshold);

			return min_val + static_cast<int32_t>(result % (range + 1));
		}

		float NextFloatInRange(float min_val, float max_val) noexcept
		{
			return min_val + NextFloat() * (max_val - min_val);
		}

		double NextDoubleInRange(double min_val, double max_val) noexcept
		{
			return min_val + NextDouble() * (max_val - min_val);
		}

		bool NextBool() noexcept
		{
			return (NextUInt32() & 1) != 0;
		}

		bool NextBool(float probability) noexcept
		{
			return NextFloat() < probability;
		}

		// Convenience methods (legacy compatibility)
		uint32_t GetUInt32() noexcept { return NextUInt32(); }
		int32_t GetInt32() noexcept { return NextInt32(); }
		float GetFloat32() noexcept { return NextFloat(); }
		double GetFloat64() noexcept { return NextDouble(); }
		
		template<typename T>
		T GetInRange(T min_val, T max_val) noexcept { return NextInRange(min_val, max_val); }

		float NextGaussian(float mean = 0.0f, float stddev = 1.0f) noexcept
		{
			static bool hasSpare = false;
			static float spare;

			if (hasSpare)
			{
				hasSpare = false;
				return spare * stddev + mean;
			}

			hasSpare = true;
			float u = NextFloat();
			float v = NextFloat();
			float mag = stddev * std::sqrt(-2.0f * std::log(u));
			spare = mag * std::cos(2.0f * PI * v);
			return mag * std::sin(2.0f * PI * v) + mean;
		}

		template<typename Iterator>
		void Shuffle(Iterator first, Iterator last) noexcept
		{
			auto n = std::distance(first, last);
			for (auto i = n - 1; i > 0; --i)
			{
				auto j = NextIntInRange(0, static_cast<int32_t>(i));
				std::swap(*(first + i), *(first + j));
			}
		}

		template<typename Container>
		void Shuffle(Container& container) noexcept
		{
			Shuffle(container.begin(), container.end());
		}

	private:
		uint64_t m_State;
		uint64_t m_Inc;

		static constexpr uint64_t PCG_MULTIPLIER = 6364136223846793005ULL;
		static constexpr uint64_t DEFAULT_SEED = 0x853c49e6748fea9bULL;
		static constexpr uint64_t DEFAULT_INC = 0xda3e39cb94b95bdbULL;

		static constexpr float FLOAT_DIVISOR = 1.0f / 16777216.0f; // 1 / 2^24
		static constexpr double DOUBLE_DIVISOR = 1.0 / 9007199254740992.0; // 1 / 2^53
		static constexpr float PI = 3.14159265358979323846f;
	};

	// Ultra-fast Xoshiro256++ generator for when maximum speed is needed
	class UltraFastRandom
	{
	public:
		using result_type = uint64_t;
		static constexpr result_type min() { return 0; }
		static constexpr result_type max() { return UINT64_MAX; }

		UltraFastRandom() noexcept
		{
			SetSeed(static_cast<uint64_t>(
				std::chrono::high_resolution_clock::now().time_since_epoch().count()
			));
		}

		explicit UltraFastRandom(uint64_t seed) noexcept
		{
			SetSeed(seed);
		}

		void SetSeed(uint64_t seed) noexcept
		{
			auto splitmix64 = [](uint64_t& z) {
				z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
				z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
				return z ^ (z >> 31);
			};

			uint64_t z = seed;
			m_State[0] = splitmix64(z);
			m_State[1] = splitmix64(z);
			m_State[2] = splitmix64(z);
			m_State[3] = splitmix64(z);
		}

		uint64_t operator()() noexcept { return NextUInt64(); }

		uint64_t NextUInt64() noexcept
		{
			const uint64_t result = RotLeft(m_State[0] + m_State[3], 23) + m_State[0];
			const uint64_t t = m_State[1] << 17;

			m_State[2] ^= m_State[0];
			m_State[3] ^= m_State[1];
			m_State[1] ^= m_State[2];
			m_State[0] ^= m_State[3];

			m_State[2] ^= t;
			m_State[3] = RotLeft(m_State[3], 45);

			return result;
		}

		uint32_t NextUInt32() noexcept
		{
			return static_cast<uint32_t>(NextUInt64() >> 32);
		}

		float NextFloat() noexcept
		{
			return static_cast<float>(NextUInt32() >> 8) * (1.0f / 16777216.0f);
		}

		template<typename T>
		T NextInRange(T min_val, T max_val) noexcept
		{
			if constexpr (std::is_integral_v<T>)
			{
				if (min_val >= max_val) return min_val;
				uint64_t range = static_cast<uint64_t>(max_val - min_val);
				return min_val + static_cast<T>(NextUInt64() % (range + 1));
			}
			else
			{
				return min_val + NextFloat() * (max_val - min_val);
			}
		}

	private:
		uint64_t m_State[4];

		static constexpr uint64_t RotLeft(uint64_t x, int k) noexcept
		{
			return (x << k) | (x >> (64 - k));
		}
	};

	// Utility functions for seeding and common operations
	namespace Random
	{
		inline uint64_t GetSeedFromCurrentTime() noexcept
		{
			return static_cast<uint64_t>(
				std::chrono::high_resolution_clock::now().time_since_epoch().count()
			);
		}

		inline thread_local FastRandom g_Random;
		inline thread_local UltraFastRandom g_UltraFastRandom;

		inline uint32_t RandomUInt32() { return g_Random.NextUInt32(); }
		inline float RandomFloat() { return g_Random.NextFloat(); }
		inline bool RandomBool() { return g_Random.NextBool(); }
		
		template<typename T>
		inline T RandomInRange(T min_val, T max_val) { return g_Random.NextInRange(min_val, max_val); }

		inline void SetGlobalSeed(uint64_t seed)
		{
			g_Random.SetSeed(seed);
			g_UltraFastRandom.SetSeed(seed);
		}
	}

}