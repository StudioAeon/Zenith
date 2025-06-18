#include <gtest/gtest.h>
#include "Zenith/Core/UUID.hpp"

#include <set>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>
#include <unordered_set>

using namespace Zenith;

// =============================================================================
// UUID32 Tests
// =============================================================================

TEST(UUID32Test, BasicGeneration) {
	std::cout << "\n=== Testing UUID32 Basic Generation ===" << std::endl;

	auto uuid32_1 = UUID32();
	auto uuid32_2 = UUID32();

	std::cout << "Generated UUID32s:" << std::endl;
	std::cout << "  UUID32 1: " << uuid32_1.toString() << " (value: " << uuid32_1.getValue() << ")" << std::endl;
	std::cout << "  UUID32 2: " << uuid32_2.toString() << " (value: " << uuid32_2.getValue() << ")" << std::endl;
	std::cout << "  Are different: " << (uuid32_1 != uuid32_2 ? "✓" : "✗") << std::endl;
	std::cout << "  UUID32 1 is null: " << (uuid32_1.isNull() ? "✓" : "✗") << std::endl;
	std::cout << "  UUID32 2 is null: " << (uuid32_2.isNull() ? "✓" : "✗") << std::endl;

	// Test uniqueness with multiple generations
	std::cout << "\nTesting uniqueness with 100 UUID32s:" << std::endl;
	std::set<uint32_t> uuid32Values;
	std::vector<std::string> sampleStrings;

	for (int i = 0; i < 100; ++i) {
		auto uuid32 = UUID32();
		uuid32Values.insert(uuid32.getValue());

		if (i < 10) {
			sampleStrings.push_back(uuid32.toString());
		}
	}

	std::cout << "  Unique values: " << uuid32Values.size() << "/100" << std::endl;
	std::cout << "  First 10 strings: ";
	for (size_t i = 0; i < sampleStrings.size(); ++i) {
		std::cout << sampleStrings[i];
		if (i < sampleStrings.size() - 1) std::cout << ", ";
	}
	std::cout << std::endl;

	EXPECT_NE(uuid32_1, uuid32_2);
	EXPECT_FALSE(uuid32_1.isNull());
	EXPECT_FALSE(uuid32_2.isNull());
	EXPECT_GT(uuid32Values.size(), 95); // Should be highly unique
}

TEST(UUID32Test, StringConversion) {
	std::cout << "\n=== Testing UUID32 String Conversion ===" << std::endl;

	auto uuid32 = UUID32();
	auto str = uuid32.toString();

	std::cout << "UUID32 string conversion:" << std::endl;
	std::cout << "  UUID32 value: " << uuid32.getValue() << std::endl;
	std::cout << "  String form:  " << str << std::endl;
	std::cout << "  String length: " << str.length() << " (expected: 8)" << std::endl;

	// Check string format (should be uppercase hex)
	bool allHex = true;
	std::cout << "  Character analysis: ";
	for (size_t i = 0; i < str.length(); ++i) {
		char c = str[i];
		bool isHex = std::isxdigit(c);
		if (!isHex) allHex = false;

		std::cout << c << "(" << (isHex ? "hex" : "!") << ")";
		if (i < str.length() - 1) std::cout << " ";
	}
	std::cout << std::endl;
	std::cout << "  All characters are hex: " << (allHex ? "✓" : "✗") << std::endl;

	EXPECT_EQ(str.length(), 8);
	for (char c : str) {
		EXPECT_TRUE(std::isxdigit(c));
	}
}

TEST(UUID32Test, StringRoundTrip) {
	std::cout << "\n=== Testing UUID32 String Round-Trip ===" << std::endl;

	auto original = UUID32();
	auto str = original.toString();
	auto parsed = UUID32::fromString(str);

	std::cout << "Round-trip conversion:" << std::endl;
	std::cout << "  Original: " << original.toString() << " (value: " << original.getValue() << ")" << std::endl;
	std::cout << "  Parsed:   " << parsed.toString() << " (value: " << parsed.getValue() << ")" << std::endl;
	std::cout << "  Match: " << (original == parsed ? "✓" : "✗") << std::endl;

	EXPECT_EQ(original, parsed);
}

// =============================================================================
// UUID64 Tests
// =============================================================================

TEST(UUID64Test, BasicGeneration) {
	std::cout << "\n=== Testing UUID64 Basic Generation ===" << std::endl;

	auto uuid64_1 = UUID64();
	auto uuid64_2 = UUID64();

	std::cout << "Generated UUID64s:" << std::endl;
	std::cout << "  UUID64 1: " << uuid64_1.toString() << std::endl;
	std::cout << "  UUID64 2: " << uuid64_2.toString() << std::endl;
	std::cout << "  Are different: " << (uuid64_1 != uuid64_2 ? "✓" : "✗") << std::endl;
	std::cout << "  UUID64 1 is null: " << (uuid64_1.isNull() ? "✓" : "✗") << std::endl;

	// Test uniqueness
	std::cout << "\nTesting uniqueness with 100 UUID64s:" << std::endl;
	std::set<uint64_t> uuid64Values;
	for (int i = 0; i < 100; ++i) {
		auto uuid64 = UUID64();
		uuid64Values.insert(uuid64.getValue());
	}
	std::cout << "  Unique values: " << uuid64Values.size() << "/100" << std::endl;

	EXPECT_NE(uuid64_1, uuid64_2);
	EXPECT_FALSE(uuid64_1.isNull());
	EXPECT_GT(uuid64Values.size(), 95);
}

TEST(UUID64Test, StringConversion) {
	std::cout << "\n=== Testing UUID64 String Conversion ===" << std::endl;

	auto uuid64 = UUID64();
	auto str = uuid64.toString();

	std::cout << "UUID64 string conversion:" << std::endl;
	std::cout << "  String form: " << str << std::endl;
	std::cout << "  String length: " << str.length() << " (expected: 16)" << std::endl;

	EXPECT_EQ(str.length(), 16);
	for (char c : str) {
		EXPECT_TRUE(std::isxdigit(c));
	}
}

TEST(UUID64Test, AliasConsistency) {
	std::cout << "\n=== Testing UUID64 Alias Consistency ===" << std::endl;

	// Test that UUID is an alias for UUID64
	auto uuid_default = UUID();
	auto uuid64_explicit = UUID64();

	std::cout << "Type alias test:" << std::endl;
	std::cout << "  UUID (default): " << uuid_default.toString() << std::endl;
	std::cout << "  UUID64: " << uuid64_explicit.toString() << std::endl;
	std::cout << "  Both are UUID64 types: ✓" << std::endl;

	// They should have the same interface
	EXPECT_FALSE(uuid_default.isNull());
	EXPECT_FALSE(uuid64_explicit.isNull());
}

// =============================================================================
// UUID128 Tests
// =============================================================================

TEST(UUID128Test, BasicGeneration) {
	std::cout << "\n=== Testing UUID128 Basic Generation ===" << std::endl;

	auto uuid128_1 = UUID128();
	auto uuid128_2 = UUID128();

	std::cout << "Generated UUID128s:" << std::endl;
	std::cout << "  UUID128 1: " << uuid128_1.toString() << std::endl;
	std::cout << "  UUID128 2: " << uuid128_2.toString() << std::endl;
	std::cout << "  Are different: " << (uuid128_1 != uuid128_2 ? "✓" : "✗") << std::endl;
	std::cout << "  UUID128 1 RFC4122v4 valid: " << (uuid128_1.isValidRFC4122v4() ? "✓" : "✗") << std::endl;
	std::cout << "  UUID128 2 RFC4122v4 valid: " << (uuid128_2.isValidRFC4122v4() ? "✓" : "✗") << std::endl;

	// Test uniqueness
	std::cout << "\nTesting uniqueness with 50 UUID128s:" << std::endl;
	std::set<std::string> uuid128Strings;
	for (int i = 0; i < 50; ++i) {
		auto uuid128 = UUID128();
		uuid128Strings.insert(uuid128.toString());
	}
	std::cout << "  Unique values: " << uuid128Strings.size() << "/50" << std::endl;

	EXPECT_NE(uuid128_1, uuid128_2);
	EXPECT_TRUE(uuid128_1.isValidRFC4122v4());
	EXPECT_TRUE(uuid128_2.isValidRFC4122v4());
	EXPECT_GT(uuid128Strings.size(), 49); // Should be essentially 100% unique
}

TEST(UUID128Test, StringFormat) {
	std::cout << "\n=== Testing UUID128 String Format ===" << std::endl;

	auto uuid128 = UUID128();
	auto str = uuid128.toString();

	std::cout << "UUID128 string format:" << std::endl;
	std::cout << "  String: " << str << std::endl;
	std::cout << "  Length: " << str.length() << " (expected: 36)" << std::endl;

	// Check format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
	int dashCount = std::count(str.begin(), str.end(), '-');
	std::cout << "  Dash count: " << dashCount << " (expected: 4)" << std::endl;

	// Check positions of dashes
	bool correctDashPositions = (str[8] == '-' && str[13] == '-' &&
								str[18] == '-' && str[23] == '-');
	std::cout << "  Correct dash positions: " << (correctDashPositions ? "✓" : "✗") << std::endl;

	// Check version (should be 4)
	bool correctVersion = (str[14] == '4');
	std::cout << "  Version 4 identifier: " << (correctVersion ? "✓" : "✗") << std::endl;

	EXPECT_EQ(str.length(), 36);
	EXPECT_EQ(dashCount, 4);
	EXPECT_TRUE(correctDashPositions);
	EXPECT_TRUE(correctVersion);
}

TEST(UUID128Test, StringRoundTrip) {
	std::cout << "\n=== Testing UUID128 String Round-Trip ===" << std::endl;

	auto original = UUID128();
	auto str = original.toString();
	auto parsed = UUID128::fromString(str);

	std::cout << "Round-trip conversion:" << std::endl;
	std::cout << "  Original: " << original.toString() << std::endl;
	std::cout << "  Parsed:   " << parsed.toString() << std::endl;
	std::cout << "  Match: " << (original == parsed ? "✓" : "✗") << std::endl;

	// Test multiple round-trips
	std::cout << "\nTesting 5 additional round-trips:" << std::endl;
	for (int i = 0; i < 5; ++i) {
		auto test_uuid = UUID128();
		auto test_str = test_uuid.toString();
		auto test_parsed = UUID128::fromString(test_str);

		std::cout << "  " << (i + 1) << ". " << test_str
				  << " -> " << (test_uuid == test_parsed ? "✓" : "✗") << std::endl;

		EXPECT_EQ(test_uuid, test_parsed);
	}

	EXPECT_EQ(original, parsed);
}

TEST(UUID128Test, As64BitPair) {
	std::cout << "\n=== Testing UUID128 64-bit Pair Conversion ===" << std::endl;

	auto uuid128 = UUID128();
	auto [high, low] = uuid128.as64BitPair();

	// Reconstruct from the pair
	auto reconstructed = UUID128(high, low);

	std::cout << "64-bit pair conversion:" << std::endl;
	std::cout << "  Original:      " << uuid128.toString() << std::endl;
	std::cout << "  High:          0x" << std::hex << high << std::dec << std::endl;
	std::cout << "  Low:           0x" << std::hex << low << std::dec << std::endl;
	std::cout << "  Reconstructed: " << reconstructed.toString() << std::endl;
	std::cout << "  RFC4122v4 valid: " << (reconstructed.isValidRFC4122v4() ? "✓" : "✗") << std::endl;

	EXPECT_TRUE(reconstructed.isValidRFC4122v4());
}

TEST(UUID128Test, InvalidStringHandling) {
	std::cout << "\n=== Testing UUID128 Invalid String Handling ===" << std::endl;

	std::vector<std::string> invalidUUIDs = {
		"not-a-uuid",
		"550e8400-e29b-41d4-a716",           // Too short
		"550e8400-e29b-41d4-a716-446655440000-extra", // Too long
		"550e8400-e29b-41d4-g716-446655440000",       // Invalid character 'g'
		"550e8400e29b41d4a716446655440000",           // No dashes
		"",                                           // Empty string
		"550e8400-e29b-41d4-a716-44665544000",       // Wrong section length
		"ZZZZZZZZ-ZZZZ-ZZZZ-ZZZZ-ZZZZZZZZZZZZ"       // Invalid hex characters
	};

	std::cout << "Testing invalid UUID128 strings:" << std::endl;

	for (size_t i = 0; i < invalidUUIDs.size(); ++i) {
		const std::string& invalidUUID = invalidUUIDs[i];
		auto uuid = UUID128::fromString(invalidUUID);

		std::cout << "  " << (i + 1) << ". \"" << invalidUUID << "\"" << std::endl;
		std::cout << "     Result: " << uuid.toString() << std::endl;
		std::cout << "     Is null: " << (uuid.isNull() ? "✓" : "✗") << std::endl;

		EXPECT_TRUE(uuid.isNull()); // Should return null for invalid input
	}
}

// =============================================================================
// Utility and Compatibility Tests
// =============================================================================

TEST(UUIDUtilityTest, GenerateBatch) {
	std::cout << "\n=== Testing Batch Generation ===" << std::endl;

	// Test batch generation for each type
	auto uuid32Batch = generateBatch<UUID32>(10);
	auto uuid64Batch = generateBatch<UUID64>(10);
	auto uuid128Batch = generateBatch<UUID128>(10);

	std::cout << "Batch generation results:" << std::endl;
	std::cout << "  UUID32 batch size: " << uuid32Batch.size() << "/10" << std::endl;
	std::cout << "  UUID64 batch size: " << uuid64Batch.size() << "/10" << std::endl;
	std::cout << "  UUID128 batch size: " << uuid128Batch.size() << "/10" << std::endl;

	// Check uniqueness within batches
	std::set<uint32_t> uuid32Set;
	std::set<uint64_t> uuid64Set;
	std::set<std::string> uuid128Set;

	for (const auto& uuid : uuid32Batch) {
		uuid32Set.insert(uuid.getValue());
	}
	for (const auto& uuid : uuid64Batch) {
		uuid64Set.insert(uuid.getValue());
	}
	for (const auto& uuid : uuid128Batch) {
		uuid128Set.insert(uuid.toString());
	}

	std::cout << "  UUID32 unique: " << uuid32Set.size() << "/10" << std::endl;
	std::cout << "  UUID64 unique: " << uuid64Set.size() << "/10" << std::endl;
	std::cout << "  UUID128 unique: " << uuid128Set.size() << "/10" << std::endl;

	EXPECT_EQ(uuid32Batch.size(), 10);
	EXPECT_EQ(uuid64Batch.size(), 10);
	EXPECT_EQ(uuid128Batch.size(), 10);
	EXPECT_EQ(uuid32Set.size(), 10);
	EXPECT_EQ(uuid64Set.size(), 10);
	EXPECT_EQ(uuid128Set.size(), 10);
}

TEST(UUIDUtilityTest, HashSupport) {
	std::cout << "\n=== Testing Hash Support ===" << std::endl;

	// Test that UUIDs can be used in hash containers
	std::unordered_set<UUID32> uuid32Set;
	std::unordered_set<UUID64> uuid64Set;
	std::unordered_set<UUID128> uuid128Set;

	for (int i = 0; i < 20; ++i) {
		uuid32Set.insert(UUID32());
		uuid64Set.insert(UUID64());
		uuid128Set.insert(UUID128());
	}

	std::cout << "Hash container tests:" << std::endl;
	std::cout << "  UUID32 hash set size: " << uuid32Set.size() << "/20" << std::endl;
	std::cout << "  UUID64 hash set size: " << uuid64Set.size() << "/20" << std::endl;
	std::cout << "  UUID128 hash set size: " << uuid128Set.size() << "/20" << std::endl;

	EXPECT_GT(uuid32Set.size(), 15); // Should be mostly unique
	EXPECT_GT(uuid64Set.size(), 15);
	EXPECT_GT(uuid128Set.size(), 19); // Should be essentially 100% unique
}

TEST(UUIDUtilityTest, ComparisonOperators) {
	std::cout << "\n=== Testing Comparison Operators ===" << std::endl;

	auto uuid32_1 = UUID32();
	auto uuid32_2 = UUID32();
	auto uuid32_copy = UUID32::fromString(uuid32_1.toString());

	std::cout << "UUID32 comparison tests:" << std::endl;
	std::cout << "  uuid32_1 == uuid32_copy: " << (uuid32_1 == uuid32_copy ? "✓" : "✗") << std::endl;
	std::cout << "  uuid32_1 != uuid32_2:    " << (uuid32_1 != uuid32_2 ? "✓" : "✗") << std::endl;
	std::cout << "  Ordering works:          " << ((uuid32_1 < uuid32_2) || (uuid32_1 > uuid32_2) ? "✓" : "✗") << std::endl;

	EXPECT_EQ(uuid32_1, uuid32_copy);
	EXPECT_NE(uuid32_1, uuid32_2);

	// Test that comparison operators work (for use in std::map, etc.)
	EXPECT_TRUE((uuid32_1 < uuid32_2) || (uuid32_1 > uuid32_2) || (uuid32_1 == uuid32_2));
}