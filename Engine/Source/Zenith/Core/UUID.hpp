#pragma once

#include "Base.hpp"
#include <string>
#include <array>
#include <vector>

namespace Zenith {

	class UUID32;
	class UUID64;
	class UUID128;

	// Base template class for UUID functionality
	template<typename Derived, typename ValueType>
	class UUIDBase
	{
	public:
		using value_type = ValueType;

		// Equality operators
		bool operator==(const Derived& other) const noexcept
		{
			return static_cast<const Derived*>(this)->getValue() == other.getValue();
		}

		bool operator!=(const Derived& other) const noexcept
		{
			return !(*this == other);
		}

		// Comparison operators for ordering (useful for maps, sets)
		bool operator<(const Derived& other) const noexcept
		{
			return static_cast<const Derived*>(this)->getValue() < other.getValue();
		}

		bool operator<=(const Derived& other) const noexcept
		{
			return !(other < *static_cast<const Derived*>(this));
		}

		bool operator>(const Derived& other) const noexcept
		{
			return other < *static_cast<const Derived*>(this);
		}

		bool operator>=(const Derived& other) const noexcept
		{
			return !(*static_cast<const Derived*>(this) < other);
		}

		std::string toString() const
		{
			return static_cast<const Derived*>(this)->toString();
		}

		bool isNull() const noexcept
		{
			return static_cast<const Derived*>(this)->getValue() == ValueType{};
		}

		friend std::ostream& operator<<(std::ostream& os, const UUIDBase& uuid)
		{
			return os << static_cast<const Derived&>(uuid).toString();
		}

	protected:
		UUIDBase() = default;
		~UUIDBase() = default;
	};

	// 32-bit UUID class
	class UUID32 : public UUIDBase<UUID32, uint32_t>
	{
	public:
		UUID32();
		explicit UUID32(uint32_t uuid) : m_UUID(uuid) {}

		UUID32(const UUID32& other) = default;
		UUID32& operator=(const UUID32& other) = default;

		UUID32(UUID32&& other) noexcept = default;
		UUID32& operator=(UUID32&& other) noexcept = default;

		// Conversion operators for backwards compatibility
		operator uint32_t() const noexcept { return m_UUID; }

		uint32_t getValue() const noexcept { return m_UUID; }

		std::string toString() const;

		static UUID32 null() { return UUID32(0); }
		static UUID32 fromString(const std::string& str);

	private:
		uint32_t m_UUID;
		static uint32_t generateRandom();
	};

	// 64-bit UUID class
	class UUID64 : public UUIDBase<UUID64, uint64_t>
	{
	public:
		UUID64();
		explicit UUID64(uint64_t uuid) : m_UUID(uuid) {}

		UUID64(const UUID64& other) = default;
		UUID64& operator=(const UUID64& other) = default;

		UUID64(UUID64&& other) noexcept = default;
		UUID64& operator=(UUID64&& other) noexcept = default;

		// Conversion operators for backwards compatibility
		operator uint64_t() const noexcept { return m_UUID; }

		uint64_t getValue() const noexcept { return m_UUID; }

		std::string toString() const;

		static UUID64 null() { return UUID64(0); }
		static UUID64 fromString(const std::string& str);

	private:
		uint64_t m_UUID;
		static uint64_t generateRandom();
	};

	// 128-bit UUID class
	class UUID128 : public UUIDBase<UUID128, std::array<uint8_t, 16>>
	{
	public:
		using uuid_array = std::array<uint8_t, 16>;

		UUID128();
		explicit UUID128(const uuid_array& uuid) : m_UUID(uuid) {}
		UUID128(uint64_t high, uint64_t low);

		UUID128(const UUID128& other) = default;
		UUID128& operator=(const UUID128& other) = default;

		UUID128(UUID128&& other) noexcept = default;
		UUID128& operator=(UUID128&& other) noexcept = default;

		const uuid_array& getValue() const noexcept { return m_UUID; }

		std::pair<uint64_t, uint64_t> as64BitPair() const noexcept;

		// RFC 4122 standard string representation: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
		std::string toString() const;

		static UUID128 null() { return UUID128(uuid_array{}); }
		static UUID128 fromString(const std::string& str);

		bool isValidRFC4122v4() const noexcept;

	private:
		uuid_array m_UUID;

		static uuid_array generateRFC4122v4();
		void applyRFC4122v4Format();
		static void applyRFC4122v4Format(uuid_array& uuid);
	};

	using UUID = UUID64;

	using UUID_32 = UUID32;
	using UUID_64 = UUID64;
	using UUID_128 = UUID128;

	using StandardUUID = UUID128;

	// Utility functions
	template<typename UUIDType>
	std::vector<UUIDType> generateBatch(size_t count);

	extern template std::vector<UUID32> generateBatch<UUID32>(size_t);
	extern template std::vector<UUID64> generateBatch<UUID64>(size_t);
	extern template std::vector<UUID128> generateBatch<UUID128>(size_t);

}

namespace std {
	template <>
	struct hash<Zenith::UUID32>
	{
		std::size_t operator()(const Zenith::UUID32& uuid) const noexcept
		{
			return std::hash<uint32_t>{}(static_cast<uint32_t>(uuid));
		}
	};

	template <>
	struct hash<Zenith::UUID64>
	{
		std::size_t operator()(const Zenith::UUID64& uuid) const noexcept
		{
			return static_cast<std::size_t>(uuid);
		}
	};

	template <>
	struct hash<Zenith::UUID128>
	{
		std::size_t operator()(const Zenith::UUID128& uuid) const noexcept
		{
			const auto& arr = uuid.getValue();
			std::size_t result = 0;
			for (size_t i = 0; i < 8 && i < arr.size(); ++i) {
				result = (result << 8) | arr[i];
			}
			return result;
		}
	};
}