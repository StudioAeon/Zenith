#pragma once

#include "Base.hpp"

#include <string>
#include <array>
#include <random>
#include <sstream>
#include <iomanip>

namespace Zenith {

	class UUID {
	public:
		using DataType = std::array<uint8_t, 16>;

		UUID();
		explicit UUID(const std::string& str);
		explicit UUID(const DataType& data);
		UUID(const UUID& other) = default;

		UUID& operator=(const UUID& other) = default;

		bool operator==(const UUID& other) const;
		bool operator!=(const UUID& other) const;
		bool operator<(const UUID& other) const;
		bool operator<=(const UUID& other) const;
		bool operator>(const UUID& other) const;
		bool operator>=(const UUID& other) const;

		std::string toString() const;
		std::string toStringWithoutDashes() const;

		const DataType& getData() const { return m_data; }

		bool isValid() const;
		static UUID generate();
		static UUID fromString(const std::string& str);

		static const UUID& nil();

	private:
		DataType m_data;

		static std::mt19937_64& getRandomGenerator();
		void generateV4();
		bool parseFromString(const std::string& str);
	};

	class UUID32 {
	public:
		UUID32();
		explicit UUID32(uint32_t value);
		UUID32(const UUID32& other) = default;

		UUID32& operator=(const UUID32& other) = default;

		bool operator==(const UUID32& other) const { return m_value == other.m_value; }
		bool operator!=(const UUID32& other) const { return m_value != other.m_value; }
		bool operator<(const UUID32& other) const { return m_value < other.m_value; }
		bool operator<=(const UUID32& other) const { return m_value <= other.m_value; }
		bool operator>(const UUID32& other) const { return m_value > other.m_value; }
		bool operator>=(const UUID32& other) const { return m_value >= other.m_value; }

		operator uint32_t() const { return m_value; }

		std::string toString() const;
		uint32_t getValue() const { return m_value; }

		static UUID32 generate();

	private:
		uint32_t m_value;

		static std::mt19937& getRandomGenerator();
	};

	std::ostream& operator<<(std::ostream& os, const UUID& uuid);
	std::istream& operator>>(std::istream& is, UUID& uuid);
	std::ostream& operator<<(std::ostream& os, const UUID32& uuid);

}

namespace std {

	template <>
	struct hash<Zenith::UUID> 
	{
		std::size_t operator()(const Zenith::UUID& uuid) const 
		{
			// Use the first 8 bytes of the UUID data for hashing
			const auto& data = uuid.getData();
			uint64_t hash_value = 0;
			for (size_t i = 0; i < 8 && i < data.size(); ++i) 
			{
				hash_value = (hash_value << 8) | data[i];
			}
			return hash<uint64_t>{}(hash_value);
		}
	};

	template <>
	struct hash<Zenith::UUID32>
	{
		std::size_t operator()(const Zenith::UUID32& uuid) const
		{
			return hash<uint32_t>{}(uuid.getValue());
		}
	};

}