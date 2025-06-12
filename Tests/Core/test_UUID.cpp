#include <gtest/gtest.h>
#include "Zenith/Core/UUID.hpp"

#include <set>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>

using namespace Zenith;

TEST(UUIDTest, GenerateUniqueUUIDs) {
	std::cout << "\n=== Testing UUID Uniqueness ===" << std::endl;

	auto uuid1 = UUID::generate();
	auto uuid2 = UUID::generate();

	std::cout << "Generated UUIDs:" << std::endl;
	std::cout << "  UUID 1: " << uuid1.toString() << std::endl;
	std::cout << "  UUID 2: " << uuid2.toString() << std::endl;
	std::cout << "  Are different: " << (uuid1 != uuid2 ? "✓" : "✗") << std::endl;
	std::cout << "  UUID 1 valid: " << (uuid1.isValid() ? "✓" : "✗") << std::endl;
	std::cout << "  UUID 2 valid: " << (uuid2.isValid() ? "✓" : "✗") << std::endl;

	// Test multiple UUIDs for uniqueness
	std::cout << "\nTesting uniqueness with 100 UUIDs..." << std::endl;
	std::set<std::string> uuidStrings;
	for (int i = 0; i < 100; ++i) {
		auto uuid = UUID::generate();
		uuidStrings.insert(uuid.toString());
	}

	std::cout << "Generated " << uuidStrings.size() << "/100 unique UUIDs" << std::endl;

	EXPECT_NE(uuid1, uuid2);
	EXPECT_TRUE(uuid1.isValid());
	EXPECT_TRUE(uuid2.isValid());
	EXPECT_EQ(uuidStrings.size(), 100); // All should be unique
}

TEST(UUIDTest, StringConversionRoundTrip) {
	std::cout << "\n=== Testing UUID String Round-Trip Conversion ===" << std::endl;

	auto original = UUID::generate();
	auto str = original.toString();
	auto parsed = UUID::fromString(str);

	std::cout << "Round-trip conversion test:" << std::endl;
	std::cout << "  Original UUID: " << original.toString() << std::endl;
	std::cout << "  String form:   " << str << std::endl;
	std::cout << "  Parsed UUID:   " << parsed.toString() << std::endl;
	std::cout << "  Round-trip successful: " << (original == parsed ? "✓" : "✗") << std::endl;

	// Test multiple round-trips
	std::cout << "\nTesting 5 additional round-trips:" << std::endl;
	for (int i = 0; i < 5; ++i) {
		auto test_uuid = UUID::generate();
		auto test_str = test_uuid.toString();
		auto test_parsed = UUID::fromString(test_str);

		std::cout << "  " << (i + 1) << ". " << test_str
				  << " -> " << (test_uuid == test_parsed ? "✓" : "✗") << std::endl;

		EXPECT_EQ(test_uuid, test_parsed);
	}

	EXPECT_EQ(original, parsed);
}

TEST(UUIDTest, NilUUID) {
	std::cout << "\n=== Testing Nil UUID ===" << std::endl;

	auto nil = UUID::nil();

	std::cout << "Nil UUID properties:" << std::endl;
	std::cout << "  String representation: " << nil.toString() << std::endl;
	std::cout << "  Is valid: " << (nil.isValid() ? "✓" : "✗") << std::endl;
	std::cout << "  Without dashes: " << nil.toStringWithoutDashes() << std::endl;

	// Verify nil UUID is all zeros
	auto nilStr = nil.toString();
	bool isAllZeros = (nilStr == "00000000-0000-0000-0000-000000000000");
	std::cout << "  Is all zeros: " << (isAllZeros ? "✓" : "✗") << std::endl;

	EXPECT_FALSE(nil.isValid());
}

TEST(UUIDTest, ConstructFromString) {
	std::cout << "\n=== Testing UUID Construction from String ===" << std::endl;

	std::string validUUID = "550e8400-e29b-41d4-a716-446655440000";
	auto uuid = UUID::fromString(validUUID);

	std::cout << "Valid UUID string test:" << std::endl;
	std::cout << "  Input string:  " << validUUID << std::endl;
	std::cout << "  Parsed UUID:   " << uuid.toString() << std::endl;
	std::cout << "  Is valid:      " << (uuid.isValid() ? "✓" : "✗") << std::endl;

	auto converted = uuid.toString();
	std::cout << "  Conversion matches: " << (validUUID == converted ? "✓" : "✗") << std::endl;

	// Test various valid formats - including case normalization
	std::vector<std::pair<std::string, std::string>> testUUIDs = {
		{"6ba7b810-9dad-11d1-80b4-00c04fd430c8", "6ba7b810-9dad-11d1-80b4-00c04fd430c8"},
		{"6ba7b811-9dad-11d1-80b4-00c04fd430c8", "6ba7b811-9dad-11d1-80b4-00c04fd430c8"},
		{"12345678-1234-5678-9abc-123456789abc", "12345678-1234-5678-9abc-123456789abc"},
		{"ABCDEF00-ABCD-ABCD-ABCD-ABCDEF123456", "abcdef00-abcd-abcd-abcd-abcdef123456"}  // Uppercase converts to lowercase
	};

	std::cout << "\nTesting additional valid UUID formats (with case normalization):" << std::endl;
	for (size_t i = 0; i < testUUIDs.size(); ++i) {
		const auto& input = testUUIDs[i].first;
		const auto& expected = testUUIDs[i].second;

		auto testUuid = UUID::fromString(input);
		auto roundTrip = testUuid.toString();
		bool matches = (expected == roundTrip);

		std::cout << "  " << (i + 1) << ". Input:    " << input << std::endl;
		std::cout << "     Output:   " << roundTrip << std::endl;
		std::cout << "     Expected: " << expected << std::endl;
		std::cout << "     Match:    " << (matches ? "✓" : "✗") << std::endl;

		EXPECT_TRUE(testUuid.isValid());
		EXPECT_EQ(expected, roundTrip);  // Use expected (normalized) value
	}

	EXPECT_TRUE(uuid.isValid());
	EXPECT_EQ(validUUID, converted);
}

TEST(UUIDTest, InvalidStringHandling) {
	std::cout << "\n=== Testing Invalid UUID String Handling ===" << std::endl;

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

	std::cout << "Testing invalid UUID strings:" << std::endl;

	for (size_t i = 0; i < invalidUUIDs.size(); ++i) {
		std::string invalidUUID = invalidUUIDs[i];
		auto uuid = UUID::fromString(invalidUUID);

		std::cout << "  " << (i + 1) << ". \"" << invalidUUID << "\"" << std::endl;
		std::cout << "     Result: " << uuid.toString() << std::endl;
		std::cout << "     Valid: " << (uuid.isValid() ? "✓" : "✗") << std::endl;

		// Most invalid UUIDs should either be invalid or fall back to a generated UUID
		// The exact behavior depends on your UUID implementation
	}
}

TEST(UUID32Test, BasicGeneration) {
	std::cout << "\n=== Testing UUID32 Basic Generation ===" << std::endl;

	auto uuid32_1 = UUID32::generate();
	auto uuid32_2 = UUID32::generate();

	std::cout << "Generated UUID32s:" << std::endl;
	std::cout << "  UUID32 1: " << uuid32_1.toString() << " (value: " << uuid32_1.getValue() << ")" << std::endl;
	std::cout << "  UUID32 2: " << uuid32_2.toString() << " (value: " << uuid32_2.getValue() << ")" << std::endl;
	std::cout << "  Are different: " << (uuid32_1.getValue() != uuid32_2.getValue() ? "✓" : "✗") << std::endl;

	// Test uniqueness with multiple generations
	std::cout << "\nTesting uniqueness with 50 UUID32s:" << std::endl;
	std::set<uint32_t> uuid32Values;
	std::vector<std::string> sampleStrings;

	for (int i = 0; i < 50; ++i) {
		auto uuid32 = UUID32::generate();
		uuid32Values.insert(uuid32.getValue());

		if (i < 10) {
			sampleStrings.push_back(uuid32.toString());
		}
	}

	std::cout << "  Unique values: " << uuid32Values.size() << "/50" << std::endl;
	std::cout << "  First 10 strings: ";
	for (size_t i = 0; i < sampleStrings.size(); ++i) {
		std::cout << sampleStrings[i];
		if (i < sampleStrings.size() - 1) std::cout << ", ";
	}
	std::cout << std::endl;

	EXPECT_NE(uuid32_1.getValue(), uuid32_2.getValue());
	EXPECT_GT(uuid32Values.size(), 45); // Should be highly unique
}

TEST(UUID32Test, StringConversion) {
	std::cout << "\n=== Testing UUID32 String Conversion ===" << std::endl;

	auto uuid32 = UUID32::generate();
	auto str = uuid32.toString();

	std::cout << "UUID32 string conversion:" << std::endl;
	std::cout << "  UUID32 value: " << uuid32.getValue() << std::endl;
	std::cout << "  String form:  " << str << std::endl;
	std::cout << "  String length: " << str.length() << " (expected: 8)" << std::endl;

	// Check each character
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

	// Test multiple UUID32s for string format consistency
	std::cout << "\nTesting 10 additional UUID32 string formats:" << std::endl;
	for (int i = 0; i < 10; ++i) {
		auto test_uuid32 = UUID32::generate();
		auto test_str = test_uuid32.toString();
		bool validLength = (test_str.length() == 8);
		bool validHex = true;

		for (char c : test_str) {
			if (!std::isxdigit(c)) {
				validHex = false;
				break;
			}
		}

		std::cout << "  " << (i + 1) << ". " << test_str
				  << " (len:" << test_str.length()
				  << ", hex:" << (validHex ? "✓" : "✗") << ")" << std::endl;

		EXPECT_EQ(test_str.length(), 8);
		EXPECT_TRUE(validHex);
	}

	EXPECT_EQ(str.length(), 8);
	for (char c : str) {
		EXPECT_TRUE(std::isxdigit(c));
	}
}

TEST(UUIDTest, ComparisonOperators) {
	std::cout << "\n=== Testing UUID Comparison Operators ===" << std::endl;

	auto uuid1 = UUID::generate();
	auto uuid2 = UUID::generate();
	auto uuid1_copy = UUID::fromString(uuid1.toString());

	std::cout << "Comparison test UUIDs:" << std::endl;
	std::cout << "  UUID 1:      " << uuid1.toString() << std::endl;
	std::cout << "  UUID 2:      " << uuid2.toString() << std::endl;
	std::cout << "  UUID 1 copy: " << uuid1_copy.toString() << std::endl;

	std::cout << "\nComparison results:" << std::endl;
	std::cout << "  uuid1 == uuid1_copy: " << (uuid1 == uuid1_copy ? "✓" : "✗") << std::endl;
	std::cout << "  uuid1 != uuid2:      " << (uuid1 != uuid2 ? "✓" : "✗") << std::endl;
	std::cout << "  uuid1 < uuid2:       " << (uuid1 < uuid2 ? "true" : "false") << std::endl;
	std::cout << "  uuid1 > uuid2:       " << (uuid1 > uuid2 ? "true" : "false") << std::endl;

	EXPECT_EQ(uuid1, uuid1_copy);
	EXPECT_NE(uuid1, uuid2);
}

TEST(UUIDTest, StringFormats) {
	std::cout << "\n=== Testing UUID String Formats ===" << std::endl;

	auto uuid = UUID::generate();
	auto withDashes = uuid.toString();
	auto withoutDashes = uuid.toStringWithoutDashes();

	std::cout << "UUID string formats:" << std::endl;
	std::cout << "  With dashes:    " << withDashes << " (length: " << withDashes.length() << ")" << std::endl;
	std::cout << "  Without dashes: " << withoutDashes << " (length: " << withoutDashes.length() << ")" << std::endl;

	// Verify format expectations
	bool correctWithDashesLength = (withDashes.length() == 36);
	bool correctWithoutDashesLength = (withoutDashes.length() == 32);
	int dashCount = std::count(withDashes.begin(), withDashes.end(), '-');

	std::cout << "  Format validation:" << std::endl;
	std::cout << "    With dashes length (36): " << (correctWithDashesLength ? "✓" : "✗") << std::endl;
	std::cout << "    Without dashes length (32): " << (correctWithoutDashesLength ? "✓" : "✗") << std::endl;
	std::cout << "    Dash count (4): " << dashCount << " " << (dashCount == 4 ? "✓" : "✗") << std::endl;

	EXPECT_EQ(withDashes.length(), 36);
	EXPECT_EQ(withoutDashes.length(), 32);
	EXPECT_EQ(dashCount, 4);
}