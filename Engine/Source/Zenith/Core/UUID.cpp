#include "znpch.hpp"
#include "UUID.hpp"

namespace Zenith {

	UUID::UUID()
	{
		generateV4();
	}

	UUID::UUID(const std::string& str)
	{
		if (!parseFromString(str))
		{
			generateV4();
		}
	}

	UUID::UUID(const DataType& data) : m_data(data) {}

	bool UUID::operator==(const UUID& other) const
	{
		return m_data == other.m_data;
	}

	bool UUID::operator!=(const UUID& other) const
	{
		return !(*this == other);
	}

	bool UUID::operator<(const UUID& other) const
	{
		return m_data < other.m_data;
	}

	bool UUID::operator<=(const UUID& other) const
	{
		return m_data <= other.m_data;
	}

	bool UUID::operator>(const UUID& other) const
	{
		return m_data > other.m_data;
	}

	bool UUID::operator>=(const UUID& other) const
	{
		return m_data >= other.m_data;
	}

	std::string UUID::toString() const
	{
		std::ostringstream oss;
		oss << std::hex << std::setfill('0');

		// Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
		for (size_t i = 0; i < 4; ++i)
		{
				oss << std::setw(2) << static_cast<int>(m_data[i]);
		}

		oss << "-";
		for (size_t i = 4; i < 6; ++i)
		{
				oss << std::setw(2) << static_cast<int>(m_data[i]);
		}

		oss << "-";
		for (size_t i = 6; i < 8; ++i)
		{
				oss << std::setw(2) << static_cast<int>(m_data[i]);
		}
		oss << "-";
		for (size_t i = 8; i < 10; ++i)
		{
				oss << std::setw(2) << static_cast<int>(m_data[i]);
		}

		oss << "-";
		for (size_t i = 10; i < 16; ++i)
		{
				oss << std::setw(2) << static_cast<int>(m_data[i]);
		}
		return oss.str();
	}

	std::string UUID::toStringWithoutDashes() const
	{
		std::ostringstream oss;
		oss << std::hex << std::setfill('0');
		for (const auto& byte : m_data)
		{
			oss << std::setw(2) << static_cast<int>(byte);
		}
		return oss.str();
	}

	bool UUID::isValid() const
	{
		return !std::all_of(m_data.begin(), m_data.end(), [](uint8_t b) { return b == 0; });
	}

	UUID UUID::generate()
	{
		return UUID();
	}

	UUID UUID::fromString(const std::string& str)
	{
		return UUID(str);
	}

	const UUID& UUID::nil() {
		static const UUID nilUuid(DataType{});
		return nilUuid;
	}

	std::mt19937_64& UUID::getRandomGenerator()
	{
		static thread_local std::mt19937_64 gen(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		return gen;
	}

	void UUID::generateV4()
	{
		auto& gen = getRandomGenerator();
		std::uniform_int_distribution<uint8_t> dis(0, 255);
		for (auto& byte : m_data)
		{
			byte = dis(gen);
		}
		m_data[6] = (m_data[6] & 0x0F) | 0x40;
		m_data[8] = (m_data[8] & 0x3F) | 0x80;
	}

	bool UUID::parseFromString(const std::string& str)
	{
		std::string cleaned = str;
		cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '-'), cleaned.end());
		if (cleaned.length() != 32)
		{
			return false;
		}
		if (!std::all_of(cleaned.begin(), cleaned.end(), ::isxdigit))
		{
			return false;
		}
		for (size_t i = 0; i < 16; ++i)
		{
			std::string byteStr = cleaned.substr(i * 2, 2);
			m_data[i] = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
		}
		return true;
	}

	// UUID32 Implementation
	UUID32::UUID32()
	{
		auto& gen = getRandomGenerator();
		std::uniform_int_distribution<uint32_t> dis;
		m_value = dis(gen);
	}

	UUID32::UUID32(uint32_t value) : m_value(value) {}

	std::string UUID32::toString() const
	{
		std::ostringstream oss;
		oss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << m_value;
		return oss.str();
	}

	UUID32 UUID32::generate()
	{
		return UUID32();
	}

	std::mt19937& UUID32::getRandomGenerator()
	{
		static thread_local std::mt19937 gen(static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
		return gen;
	}

	// Stream operators
	std::ostream& operator<<(std::ostream& os, const UUID& uuid)
	{
		return os << uuid.toString();
	}

	std::istream& operator>>(std::istream& is, UUID& uuid)
	{
		std::string str;
		is >> str;
		uuid = UUID::fromString(str);
		return is;
	}

	std::ostream& operator<<(std::ostream& os, const UUID32& uuid)
	{
		return os << uuid.toString();
	}

}