#include "EditorLayer.hpp"

#include <iostream>

namespace Zenith {

	EditorLayer::EditorLayer()
	{}

	EditorLayer::~EditorLayer()
	{}

	void EditorLayer::OnAttach()
	{
		UpdateWindowTitle("Untitled Project");

		auto uuid = UUID::generate();
		ZN_INFO("Generated UUID: {}", uuid);

		ZN_WARN("=== FastRandom Testing ===");

		// Test 1: Basic random generation
		FastRandom rng;
		ZN_WARN("Random uint32: {}", rng.NextUInt32());
		ZN_WARN("Random float [0,1): {}", rng.NextFloat());
		ZN_WARN("Random double [0,1): {}", rng.NextDouble());
		ZN_WARN("Random bool: {}", rng.NextBool());

		// Test 2: Range-based generation
		ZN_WARN("Random int [1,100]: {}", rng.NextInRange(1, 100));
		ZN_WARN("Random float [10.0f, 20.0f]: {}", rng.NextInRange(10.0f, 20.0f));

		// Test 3: Seeded generation (reproducible)
		FastRandom seededRng(12345);
		ZN_WARN("Seeded random (seed=12345): {}", seededRng.NextUInt32());

		// Test 4: Global utility functions
		Random::SetGlobalSeed(54321);
		ZN_WARN("Global random uint32: {}", Random::RandomUInt32());
		ZN_WARN("Global random float: {}", Random::RandomFloat());
		ZN_WARN("Global random bool: {}", Random::RandomBool());
		ZN_WARN("Global random in range [1,10]: {}", Random::RandomInRange(1, 10));

		// Test 5: Gaussian distribution
		ZN_WARN("Gaussian (mean=0, stddev=1): {}", rng.NextGaussian());
		ZN_WARN("Gaussian (mean=100, stddev=15): {}", rng.NextGaussian(100.0f, 15.0f));

		// Test 6: UltraFastRandom
		UltraFastRandom ultraRng;
		ZN_WARN("UltraFast uint64: {}", ultraRng.NextUInt64());
		ZN_WARN("UltraFast uint32: {}", ultraRng.NextUInt32());
		ZN_WARN("UltraFast float: {}", ultraRng.NextFloat());
		ZN_WARN("UltraFast range [1,1000]: {}", ultraRng.NextInRange(1, 1000));

		// Test 7: Container shuffling
		std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
		ZN_WARN("Original vector: [1,2,3,4,5,6,7,8,9,10]");
		rng.Shuffle(numbers);
		std::string shuffled = "[";
		for (size_t i = 0; i < numbers.size(); ++i) {
				shuffled += std::to_string(numbers[i]);
				if (i < numbers.size() - 1) shuffled += ",";
		}
		shuffled += "]";
		ZN_WARN("Shuffled vector: {}", shuffled);

		// Test 8: Performance comparison (generate 1 million numbers)
		auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 1000000; ++i) {
				rng.NextUInt32();
		}
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		ZN_WARN("FastRandom: Generated 1M numbers in {} microseconds", duration.count());

		start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 1000000; ++i) {
				ultraRng.NextUInt64();
		}
		end = std::chrono::high_resolution_clock::now();
		duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		ZN_WARN("UltraFastRandom: Generated 1M numbers in {} microseconds", duration.count());

		ZN_WARN("=== FastRandom Testing Complete ===");


	}

	void EditorLayer::OnDetach()
	{}

	void EditorLayer::UpdateWindowTitle(const std::string& sceneName)
	{
		std::string title = sceneName + " - Zenith-Editor - " + Application::GetPlatformName() + " (" + Application::GetConfigurationName() + ")";
		Application::Get().GetWindow().SetTitle(title);
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{}

	void EditorLayer::OnEvent(Event& e)
	{}


}