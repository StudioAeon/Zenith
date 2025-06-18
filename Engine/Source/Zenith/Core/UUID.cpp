#include "znpch.hpp"
#include "UUID.hpp"

namespace Zenith {

	// UUID32 Implementation
	UUID32::UUID32() : m_UUID(generateRandom()) {}

	uint32_t UUID32::generateRandom()
	{
		static thread_local std::random_device rd;
		static thread_local std::mt19937 gen(rd());
		static thread_local std::uniform_int_distribution<uint32_t> dis;
		return dis(gen);
	}

	std::string UUID32::toString() const
	{
		std::stringstream ss;
		ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << m_UUID;
		return ss.str();
	}

	UUID32 UUID32::fromString(const std::string& str)
	{
		if (str.empty()) return null();

		try {
			return UUID32(static_cast<uint32_t>(std::stoull(str, nullptr, 16)));
		} catch (...) {
			return null();
		}
	}

	// UUID64 Implementation
	UUID64::UUID64() : m_UUID(generateRandom()) {}

	uint64_t UUID64::generateRandom()
	{
		static thread_local std::random_device rd;
		static thread_local std::mt19937_64 gen(rd());
		static thread_local std::uniform_int_distribution<uint64_t> dis;
		return dis(gen);
	}

	std::string UUID64::toString() const
	{
		std::stringstream ss;
		ss << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << m_UUID;
		return ss.str();
	}

	UUID64 UUID64::fromString(const std::string& str)
	{
		if (str.empty()) return null();

		try {
			return UUID64(std::stoull(str, nullptr, 16));
		} catch (...) {
			return null();
		}
	}

	// UUID128 Implementation
	UUID128::UUID128() : m_UUID(generateRFC4122v4()) {}

	UUID128::UUID128(uint64_t high, uint64_t low)
	{
		// Portable way to set bytes without endianness issues
		for (int i = 0; i < 8; ++i) {
			m_UUID[i] = static_cast<uint8_t>(high >> (56 - i * 8));
			m_UUID[i + 8] = static_cast<uint8_t>(low >> (56 - i * 8));
		}
		applyRFC4122v4Format();
	}

	std::pair<uint64_t, uint64_t> UUID128::as64BitPair() const noexcept
	{
		uint64_t high = 0, low = 0;
		for (int i = 0; i < 8; ++i) {
			high = (high << 8) | m_UUID[i];
			low = (low << 8) | m_UUID[i + 8];
		}
		return {high, low};
	}

	std::string UUID128::toString() const
	{
		std::stringstream ss;
		ss << std::hex << std::uppercase << std::setfill('0');

		for (size_t i = 0; i < 4; ++i)
			ss << std::setw(2) << static_cast<unsigned>(m_UUID[i]);
		ss << '-';
		for (size_t i = 4; i < 6; ++i)
			ss << std::setw(2) << static_cast<unsigned>(m_UUID[i]);
		ss << '-';
		for (size_t i = 6; i < 8; ++i)
			ss << std::setw(2) << static_cast<unsigned>(m_UUID[i]);
		ss << '-';
		for (size_t i = 8; i < 10; ++i)
			ss << std::setw(2) << static_cast<unsigned>(m_UUID[i]);
		ss << '-';
		for (size_t i = 10; i < 16; ++i)
			ss << std::setw(2) << static_cast<unsigned>(m_UUID[i]);

		return ss.str();
	}

	UUID128 UUID128::fromString(const std::string& str)
	{
		if (str.length() != 36) return null();

		uuid_array result{};
		size_t byteIndex = 0;

		for (size_t i = 0; i < str.length(); ++i)
		{
			if (str[i] == '-') continue;

			if (byteIndex >= 16) return null();

			char hex[3] = {str[i], str[i+1], '\0'};
			try {
				result[byteIndex] = static_cast<uint8_t>(std::stoi(hex, nullptr, 16));
				++byteIndex;
				++i;
			} catch (...) {
				return null();
			}
		}

		return byteIndex == 16 ? UUID128(result) : null();
	}

	bool UUID128::isValidRFC4122v4() const noexcept
	{
		if ((m_UUID[6] & 0xF0) != 0x40) return false;
		if ((m_UUID[8] & 0xC0) != 0x80) return false;
		return true;
	}

	UUID128::uuid_array UUID128::generateRFC4122v4()
	{
		static thread_local std::random_device rd;
		static thread_local std::mt19937_64 gen(rd());
		static thread_local std::uniform_int_distribution<unsigned int> dis(0, 255);

		uuid_array result;
		for (auto& byte : result)
			byte = static_cast<uint8_t>(dis(gen));

		applyRFC4122v4Format(result);
		return result;
	}

	void UUID128::applyRFC4122v4Format()
	{
		applyRFC4122v4Format(m_UUID);
	}

	void UUID128::applyRFC4122v4Format(uuid_array& uuid)
	{
		uuid[6] = (uuid[6] & 0x0F) | 0x40;
		uuid[8] = (uuid[8] & 0x3F) | 0x80;
	}

	// Utility functions
	template<typename UUIDType>
	std::vector<UUIDType> generateBatch(size_t count)
	{
		std::vector<UUIDType> result;
		result.reserve(count);

		for (size_t i = 0; i < count; ++i)
			result.emplace_back();

		return result;
	}

	// Explicit template instantiations
	template std::vector<UUID32> generateBatch<UUID32>(size_t);
	template std::vector<UUID64> generateBatch<UUID64>(size_t);
	template std::vector<UUID128> generateBatch<UUID128>(size_t);

}