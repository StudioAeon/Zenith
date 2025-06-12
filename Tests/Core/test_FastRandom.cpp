#include <gtest/gtest.h>
#include "Zenith/Core/FastRandom.hpp"

#include <set>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>

using namespace Zenith;

TEST(FastRandomTest, RangeGeneration) {
	std::cout << "\n=== Testing Range Generation [1,10] ===" << std::endl;
	FastRandom rng(12345); // Seeded for reproducibility

	std::cout << "Generating first 20 values:" << std::endl;
	std::vector<int> sampleValues;

	for (int i = 0; i < 1000; ++i) {
		int val = rng.NextInRange(1, 10);

		// Show first 20 values
		if (i < 20) {
			sampleValues.push_back(val);
			std::cout << val << " ";
			if ((i + 1) % 10 == 0) std::cout << std::endl;
		}

		EXPECT_GE(val, 1);
		EXPECT_LE(val, 10);
	}

	std::cout << "\nRange generation test completed - all 1000 values within [1,10]" << std::endl;
}

TEST(FastRandomTest, ReproducibleSequences) {
	std::cout << "\n=== Testing Reproducible Sequences (seed=42) ===" << std::endl;
	FastRandom rng1(42);
	FastRandom rng2(42);

	std::cout << "Comparing first 10 pairs:" << std::endl;
	std::cout << "RNG1    RNG2    Match?" << std::endl;

	for (int i = 0; i < 100; ++i) {
		uint32_t val1 = rng1.NextUInt32();
		uint32_t val2 = rng2.NextUInt32();

		if (i < 10) {
			std::cout << std::setw(8) << val1 << " "
					  << std::setw(8) << val2 << " "
					  << (val1 == val2 ? "✓" : "✗") << std::endl;
		}

		EXPECT_EQ(val1, val2);
	}

	std::cout << "All 100 sequence pairs matched perfectly!" << std::endl;
}

TEST(FastRandomTest, FloatRange) {
	std::cout << "\n=== Testing Float Range [0.0, 1.0) ===" << std::endl;
	FastRandom rng(12345);

	std::cout << "First 10 float values:" << std::endl;
	float min_val = 1.0f, max_val = 0.0f;

	for (int i = 0; i < 1000; ++i) {
		float val = rng.NextFloat();

		if (i < 10) {
			std::cout << std::fixed << std::setprecision(6) << val << " ";
			if ((i + 1) % 5 == 0) std::cout << std::endl;
		}

		min_val = std::min(min_val, val);
		max_val = std::max(max_val, val);

		EXPECT_GE(val, 0.0f);
		EXPECT_LT(val, 1.0f);
	}

	std::cout << "\nFloat range stats:" << std::endl;
	std::cout << "  Min value: " << std::fixed << std::setprecision(6) << min_val << std::endl;
	std::cout << "  Max value: " << std::fixed << std::setprecision(6) << max_val << std::endl;
	std::cout << "All 1000 float values in range [0.0, 1.0)" << std::endl;
}

TEST(FastRandomTest, FloatInRange) {
	std::cout << "\n=== Testing Float Range [10.0, 20.0] ===" << std::endl;
	FastRandom rng(12345);

	std::cout << "First 10 values in range [10.0, 20.0]:" << std::endl;
	float sum = 0.0f;

	for (int i = 0; i < 1000; ++i) {
		float val = rng.NextFloatInRange(10.0f, 20.0f);

		if (i < 10) {
			std::cout << std::fixed << std::setprecision(3) << val << " ";
		}

		sum += val;
		EXPECT_GE(val, 10.0f);
		EXPECT_LE(val, 20.0f);
	}

	float average = sum / 1000.0f;
	std::cout << "\nAverage of 1000 values: " << std::fixed << std::setprecision(3)
			  << average << " (expected ~15.0)" << std::endl;
}

TEST(FastRandomTest, BoolGeneration) {
	std::cout << "\n=== Testing Bool Generation (50% probability) ===" << std::endl;
	FastRandom rng(12345);
	int trueCount = 0;
	int falseCount = 0;

	std::cout << "First 20 boolean values: ";

	for (int i = 0; i < 1000; ++i) {
		bool val = rng.NextBool();

		if (i < 20) {
			std::cout << (val ? "T" : "F");
		}

		if (val) trueCount++;
		else falseCount++;
	}

	std::cout << std::endl;
	std::cout << "Bool distribution:" << std::endl;
	std::cout << "  True:  " << trueCount << " (" << (trueCount * 100.0f / 1000.0f) << "%)" << std::endl;
	std::cout << "  False: " << falseCount << " (" << (falseCount * 100.0f / 1000.0f) << "%)" << std::endl;

	// Should have roughly equal distribution (allow some variance)
	EXPECT_GT(trueCount, 400);
	EXPECT_GT(falseCount, 400);
}

TEST(FastRandomTest, BoolWithProbability) {
	std::cout << "\n=== Testing Bool with 25% Probability ===" << std::endl;
	FastRandom rng(12345);
	int trueCount = 0;

	std::cout << "First 40 values (25% true probability): ";

	// 25% probability of true
	for (int i = 0; i < 1000; ++i) {
		bool val = rng.NextBool(0.25f);

		if (i < 40) {
			std::cout << (val ? "T" : "F");
			if ((i + 1) % 10 == 0) std::cout << " ";
		}

		if (val) {
			trueCount++;
		}
	}

	float percentage = (trueCount * 100.0f) / 1000.0f;
	std::cout << std::endl;
	std::cout << "Results: " << trueCount << "/1000 true (" << std::fixed << std::setprecision(1)
			  << percentage << "%, expected ~25%)" << std::endl;

	// Should be roughly 25% (allow some variance)
	EXPECT_GT(trueCount, 200);
	EXPECT_LT(trueCount, 300);
}

TEST(FastRandomTest, GaussianDistribution) {
	std::cout << "\n=== Testing Gaussian Distribution (mean=0, stddev=1) ===" << std::endl;
	FastRandom rng(12345);
	std::vector<float> values;

	std::cout << "First 10 Gaussian values: ";

	for (int i = 0; i < 1000; ++i) {
		float val = rng.NextGaussian(0.0f, 1.0f);
		values.push_back(val);

		if (i < 10) {
			std::cout << std::fixed << std::setprecision(3) << val << " ";
		}
	}

	// Basic sanity checks for Gaussian distribution
	float mean = std::accumulate(values.begin(), values.end(), 0.0f) / values.size();

	// Calculate standard deviation
	float variance = 0.0f;
	for (float val : values) {
		variance += (val - mean) * (val - mean);
	}
	variance /= values.size();
	float stddev = std::sqrt(variance);

	// Most values should be within 3 standard deviations
	int withinThreeSigma = 0;
	for (float val : values) {
		if (std::abs(val) <= 3.0f) {
			withinThreeSigma++;
		}
	}

	std::cout << std::endl;
	std::cout << "Gaussian statistics:" << std::endl;
	std::cout << "  Mean: " << std::fixed << std::setprecision(4) << mean << " (expected ~0.0)" << std::endl;
	std::cout << "  Std Dev: " << stddev << " (expected ~1.0)" << std::endl;
	std::cout << "  Within 3σ: " << withinThreeSigma << "/1000 (" << (withinThreeSigma / 10.0f) << "%)" << std::endl;

	EXPECT_NEAR(mean, 0.0f, 0.2f); // Should be close to 0
	EXPECT_GT(withinThreeSigma, 990); // Should be > 99%
}

TEST(FastRandomTest, VectorShuffle) {
	std::cout << "\n=== Testing Vector Shuffle ===" << std::endl;
	FastRandom rng(12345);
	std::vector<int> original = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	std::vector<int> shuffled = original;

	std::cout << "Original: ";
	for (int val : original) std::cout << val << " ";
	std::cout << std::endl;

	rng.Shuffle(shuffled);

	std::cout << "Shuffled: ";
	for (int val : shuffled) std::cout << val << " ";
	std::cout << std::endl;

	// Should contain same elements
	std::sort(shuffled.begin(), shuffled.end());
	std::cout << "After sort: ";
	for (int val : shuffled) std::cout << val << " ";
	std::cout << std::endl;

	EXPECT_EQ(original, shuffled);
	std::cout << "Shuffle preserves all elements ✓" << std::endl;
}

TEST(FastRandomTest, EdgeCaseRanges) {
	std::cout << "\n=== Testing Edge Case Ranges ===" << std::endl;
	FastRandom rng(12345);

	std::cout << "Single value range [5,5]: ";
	// Single value range
	for (int i = 0; i < 10; ++i) {
		int val = rng.NextInRange(5, 5);
		std::cout << val << " ";
		EXPECT_EQ(val, 5);
	}
	std::cout << std::endl;

	std::cout << "Large range [-1M, 1M] - first 10 values: ";
	// Large range
	for (int i = 0; i < 10; ++i) {
		int val = rng.NextInRange(-1000000, 1000000);
		std::cout << val << " ";
		EXPECT_GE(val, -1000000);
		EXPECT_LE(val, 1000000);
	}
	std::cout << std::endl;
}

TEST(UltraFastRandomTest, BasicFunctionality) {
	std::cout << "\n=== Testing UltraFastRandom Uniqueness ===" << std::endl;
	UltraFastRandom rng(12345);

	// Test that it generates different values
	std::set<uint64_t> values;
	std::cout << "First 10 UltraFast uint64 values:" << std::endl;

	for (int i = 0; i < 100; ++i) {
		uint64_t val = rng.NextUInt64();
		values.insert(val);

		if (i < 10) {
			std::cout << val << std::endl;
		}
	}

	std::cout << "Unique values: " << values.size() << "/100 (" << values.size() << "% unique)" << std::endl;

	// Should generate mostly unique values
	EXPECT_GT(values.size(), 95);
}

TEST(UltraFastRandomTest, RangeGeneration) {
	std::cout << "\n=== Testing UltraFastRandom Range [1,100] ===" << std::endl;
	UltraFastRandom rng(12345);

	std::cout << "First 20 values: ";

	for (int i = 0; i < 1000; ++i) {
		int val = rng.NextInRange(1, 100);

		if (i < 20) {
			std::cout << val << " ";
			if ((i + 1) % 10 == 0) std::cout << std::endl << "                 ";
		}

		EXPECT_GE(val, 1);
		EXPECT_LE(val, 100);
	}

	std::cout << std::endl << "All 1000 UltraFast values in range [1,100] ✓" << std::endl;
}

TEST(RandomGlobalFunctionsTest, GlobalUtilities) {
	std::cout << "\n=== Testing Global Random Functions ===" << std::endl;
	Random::SetGlobalSeed(54321);

	// Test global functions work
	uint32_t val1 = Random::RandomUInt32();
	float val2 = Random::RandomFloat();
	bool val3 = Random::RandomBool();
	int val4 = Random::RandomInRange(1, 10);

	std::cout << "Global function results:" << std::endl;
	std::cout << "  RandomUInt32(): " << val1 << std::endl;
	std::cout << "  RandomFloat():  " << std::fixed << std::setprecision(6) << val2 << std::endl;
	std::cout << "  RandomBool():   " << (val3 ? "true" : "false") << std::endl;
	std::cout << "  RandomInRange(1,10): " << val4 << std::endl;

	EXPECT_GE(val2, 0.0f);
	EXPECT_LT(val2, 1.0f);
	EXPECT_GE(val4, 1);
	EXPECT_LE(val4, 10);

	std::cout << "All global functions working correctly ✓" << std::endl;
}