#pragma once

#include <string>
#include <string_view>
#include <array>

namespace Zenith {

	class CRC32Hash;
	class FNVHash;
	class SHA256Hash;

	template<typename Derived, typename ValueType>
	class HashBase
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

		// Comparison operators for ordering
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

		// Conversion to underlying type
		operator ValueType() const noexcept
		{
			return static_cast<const Derived*>(this)->getValue();
		}

		ValueType getValue() const noexcept
		{
			return static_cast<const Derived*>(this)->getValue();
		}

	protected:
		HashBase() = default;
		~HashBase() = default;
	};

	// CRC32 Hash Algorithm
	class CRC32Hash : public HashBase<CRC32Hash, uint32_t>
	{
	public:
		CRC32Hash() : m_hash(0) {}
		explicit CRC32Hash(uint32_t hash) : m_hash(hash) {}

		explicit CRC32Hash(const char* str);
		explicit CRC32Hash(const std::string& str);
		explicit CRC32Hash(std::string_view str);

		CRC32Hash(const CRC32Hash& other) = default;
		CRC32Hash& operator=(const CRC32Hash& other) = default;

		CRC32Hash(CRC32Hash&& other) noexcept = default;
		CRC32Hash& operator=(CRC32Hash&& other) noexcept = default;

		uint32_t getValue() const noexcept { return m_hash; }

		static uint32_t compute(const char* str);
		static uint32_t compute(const std::string& str);
		static uint32_t compute(std::string_view str);

		void reset() { m_hash = 0xFFFFFFFFu; }
		void update(const char* data, size_t length);
		void update(std::string_view str) { update(str.data(), str.length()); }
		uint32_t finalize();

	private:
		uint32_t m_hash;
		static const std::array<uint32_t, 256>& getCRC32Table();
	};

	// FNV Hash Algorithm
	class FNVHash : public HashBase<FNVHash, uint32_t>
	{
	public:
		FNVHash() : m_hash(OFFSET_BASIS) {}
		explicit FNVHash(uint32_t hash) : m_hash(hash) {}

		explicit constexpr FNVHash(std::string_view str) : m_hash(compute(str)) {}

		FNVHash(const FNVHash& other) = default;
		FNVHash& operator=(const FNVHash& other) = default;

		FNVHash(FNVHash&& other) noexcept = default;
		FNVHash& operator=(FNVHash&& other) noexcept = default;

		uint32_t getValue() const noexcept { return m_hash; }

		static constexpr uint32_t compute(std::string_view str)
		{
			constexpr uint32_t FNV_PRIME = 16777619u;
			constexpr uint32_t OFFSET_BASIS = 2166136261u;

			const size_t length = str.length();
			const char* data = str.data();

			uint32_t hash = OFFSET_BASIS;
			for (size_t i = 0; i < length; ++i)
			{
				hash ^= *data++;
				hash *= FNV_PRIME;
			}
			hash ^= '\0';
			hash *= FNV_PRIME;

			return hash;
		}

		void reset() { m_hash = OFFSET_BASIS; }
		constexpr void update(std::string_view str)
		{
			for (char c : str)
			{
				m_hash ^= static_cast<uint8_t>(c);
				m_hash *= FNV_PRIME;
			}
		}
		constexpr void update(char c)
		{
			m_hash ^= static_cast<uint8_t>(c);
			m_hash *= FNV_PRIME;
		}
		constexpr uint32_t finalize()
		{
			m_hash ^= '\0';
			m_hash *= FNV_PRIME;
			return m_hash;
		}

	private:
		uint32_t m_hash;
		static constexpr uint32_t FNV_PRIME = 16777619u;
		static constexpr uint32_t OFFSET_BASIS = 2166136261u;
	};

	// SHA-256 Hash Algorithm
	class SHA256Hash : public HashBase<SHA256Hash, std::array<uint8_t, 32>>
	{
	public:
		using HashValue = std::array<uint8_t, 32>;

		SHA256Hash() { reset(); }
		explicit SHA256Hash(const HashValue& hash) : m_hash(hash) {}

		explicit SHA256Hash(const char* str);
		explicit SHA256Hash(const std::string& str);
		explicit SHA256Hash(std::string_view str);

		SHA256Hash(const SHA256Hash& other) = default;
		SHA256Hash& operator=(const SHA256Hash& other) = default;

		SHA256Hash(SHA256Hash&& other) noexcept = default;
		SHA256Hash& operator=(SHA256Hash&& other) noexcept = default;

		const HashValue& getValue() const noexcept { return m_hash; }

		static HashValue compute(const char* str);
		static HashValue compute(const std::string& str);
		static HashValue compute(std::string_view str);
		static HashValue compute(const uint8_t* data, size_t length);

		void reset();
		void update(const char* data, size_t length);
		void update(std::string_view str) { update(reinterpret_cast<const uint8_t*>(str.data()), str.length()); }
		void update(const uint8_t* data, size_t length);
		HashValue finalize();

		// Utility methods
		std::string toHexString() const;
		std::string toHexString(bool uppercase) const;
		static SHA256Hash fromHexString(const std::string& hex);

	private:
		HashValue m_hash{};
		std::array<uint32_t, 8> m_state{};
		std::array<uint8_t, 64> m_buffer{};
		uint64_t m_bitlen = 0;
		size_t m_buflen = 0;

		void processBlock();
		static constexpr uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
		static constexpr uint32_t choose(uint32_t e, uint32_t f, uint32_t g) { return (e & f) ^ (~e & g); }
		static constexpr uint32_t majority(uint32_t a, uint32_t b, uint32_t c) { return (a & b) ^ (a & c) ^ (b & c); }
		static constexpr uint32_t sig0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
		static constexpr uint32_t sig1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

		static constexpr std::array<uint32_t, 64> K = {
			0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
			0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
			0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
			0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
			0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
			0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
			0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
			0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
		};
	};

	// Original Hash class for backwards compatibility
	class Hash
	{
	public:
		static constexpr uint32_t GenerateFNVHash(std::string_view str)
		{
			return FNVHash::compute(str);
		}

		static uint32_t CRC32(const char* str)
		{
			return CRC32Hash::compute(str);
		}

		static uint32_t CRC32(const std::string& string)
		{
			return CRC32Hash::compute(string);
		}

		static uint32_t CRC32(std::string_view str)
		{
			return CRC32Hash::compute(str);
		}

		static SHA256Hash::HashValue SHA256(const char* str)
		{
			return SHA256Hash::compute(str);
		}

		static SHA256Hash::HashValue SHA256(const std::string& string)
		{
			return SHA256Hash::compute(string);
		}

		static SHA256Hash::HashValue SHA256(std::string_view str)
		{
			return SHA256Hash::compute(str);
		}
	};

	using CRC32 = CRC32Hash;
	using FNV = FNVHash;
	using FNV32 = FNVHash;
	using SHA256 = SHA256Hash;

}

namespace std {
	template <>
	struct hash<Zenith::CRC32Hash>
	{
		std::size_t operator()(const Zenith::CRC32Hash& h) const noexcept
		{
			return static_cast<std::size_t>(h.getValue());
		}
	};

	template <>
	struct hash<Zenith::FNVHash>
	{
		std::size_t operator()(const Zenith::FNVHash& h) const noexcept
		{
			return static_cast<std::size_t>(h.getValue());
		}
	};

	template <>
	struct hash<Zenith::SHA256Hash>
	{
		std::size_t operator()(const Zenith::SHA256Hash& h) const noexcept
		{
			const auto& value = h.getValue();
			// Use first 8 bytes as hash, combining them into a single size_t
			std::size_t result = 0;
			for (size_t i = 0; i < std::min(sizeof(std::size_t), value.size()); ++i) {
				result = (result << 8) | value[i];
			}
			return result;
		}
	};
}