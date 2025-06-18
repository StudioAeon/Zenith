#include "znpch.hpp"
#include "Hash.hpp"

namespace {
	constexpr auto gen_crc32_table()
	{
		constexpr int num_bytes = 256;
		constexpr int num_iterations = 8;
		constexpr uint32_t polynomial = 0xEDB88320;

		std::array<uint32_t, num_bytes> crc32_table{};

		for (int byte = 0; byte < num_bytes; ++byte)
		{
			uint32_t crc = static_cast<uint32_t>(byte);
			for (int i = 0; i < num_iterations; ++i)
			{
				int mask = -static_cast<int>(crc & 1);
				crc = (crc >> 1) ^ (polynomial & static_cast<uint32_t>(mask));
			}

			crc32_table[byte] = crc;
		}

		return crc32_table;
	}

	static constexpr auto crc32_table = gen_crc32_table();
	static_assert(
		crc32_table.size() == 256 &&
		crc32_table[1] == 0x77073096 &&
		crc32_table[255] == 0x2D02EF8D,
		"gen_crc32_table generated unexpected result."
	);
}

namespace Zenith {

	// CRC32Hash Implementation
	CRC32Hash::CRC32Hash(const char* str)
		: m_hash(compute(str))
	{
	}

	CRC32Hash::CRC32Hash(const std::string& str)
		: m_hash(compute(str))
	{
	}

	CRC32Hash::CRC32Hash(std::string_view str)
		: m_hash(compute(str))
	{
	}

	const std::array<uint32_t, 256>& CRC32Hash::getCRC32Table()
	{
		return crc32_table;
	}

	uint32_t CRC32Hash::compute(const char* str)
	{
		auto crc = 0xFFFFFFFFu;

		for (auto i = 0u; auto c = str[i]; ++i) {
			crc = crc32_table[(crc ^ static_cast<uint8_t>(c)) & 0xFF] ^ (crc >> 8);
		}

		return ~crc;
	}

	uint32_t CRC32Hash::compute(const std::string& str)
	{
		return compute(str.c_str());
	}

	uint32_t CRC32Hash::compute(std::string_view str)
	{
		auto crc = 0xFFFFFFFFu;

		for (char c : str) {
			crc = crc32_table[(crc ^ static_cast<uint8_t>(c)) & 0xFF] ^ (crc >> 8);
		}

		return ~crc;
	}

	void CRC32Hash::update(const char* data, size_t length)
	{
		for (size_t i = 0; i < length; ++i) {
			m_hash = crc32_table[(m_hash ^ static_cast<uint8_t>(data[i])) & 0xFF] ^ (m_hash >> 8);
		}
	}

	uint32_t CRC32Hash::finalize()
	{
		uint32_t result = ~m_hash;
		reset();
		return result;
	}

	// SHA256Hash Implementation
	constexpr std::array<uint32_t, 64> SHA256Hash::K;

	SHA256Hash::SHA256Hash(const char* str)
		: m_hash(compute(str))
	{
	}

	SHA256Hash::SHA256Hash(const std::string& str)
		: m_hash(compute(str))
	{
	}

	SHA256Hash::SHA256Hash(std::string_view str)
		: m_hash(compute(str))
	{
	}

	SHA256Hash::HashValue SHA256Hash::compute(const char* str)
	{
		return compute(std::string_view(str));
	}

	SHA256Hash::HashValue SHA256Hash::compute(const std::string& str)
	{
		return compute(std::string_view(str));
	}

	SHA256Hash::HashValue SHA256Hash::compute(std::string_view str)
	{
		SHA256Hash hasher;
		hasher.update(str);
		return hasher.finalize();
	}

	SHA256Hash::HashValue SHA256Hash::compute(const uint8_t* data, size_t length)
	{
		SHA256Hash hasher;
		hasher.update(data, length);
		return hasher.finalize();
	}

	void SHA256Hash::reset()
	{
		m_state = {
			0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
			0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
		};

		m_buffer.fill(0);
		m_bitlen = 0;
		m_buflen = 0;
	}

	void SHA256Hash::update(const char* data, size_t length)
	{
		update(reinterpret_cast<const uint8_t*>(data), length);
	}

	void SHA256Hash::update(const uint8_t* data, size_t length)
	{
		for (size_t i = 0; i < length; ++i) {
			m_buffer[m_buflen] = data[i];
			m_buflen++;

			if (m_buflen == 64) {
				processBlock();
				m_bitlen += 512;
				m_buflen = 0;
			}
		}
	}

	SHA256Hash::HashValue SHA256Hash::finalize()
	{
		size_t i = m_buflen;

		// Pad whatever data is left in the buffer
		if (m_buflen < 56) {
			m_buffer[i++] = 0x80;
			while (i < 56) {
				m_buffer[i++] = 0x00;
			}
		} else {
			m_buffer[i++] = 0x80;
			while (i < 64) {
				m_buffer[i++] = 0x00;
			}
			processBlock();
			std::fill(m_buffer.begin(), m_buffer.begin() + 56, 0);
		}

		m_bitlen += m_buflen * 8;
		for (int i = 7; i >= 0; --i) {
			m_buffer[56 + i] = static_cast<uint8_t>(m_bitlen >> (8 * (7 - i)));
		}

		processBlock();

		// Produce the final hash value as a 256-bit number (big-endian)
		HashValue result;
		for (size_t i = 0; i < 8; ++i) {
			result[i * 4 + 0] = static_cast<uint8_t>(m_state[i] >> 24);
			result[i * 4 + 1] = static_cast<uint8_t>(m_state[i] >> 16);
			result[i * 4 + 2] = static_cast<uint8_t>(m_state[i] >> 8);
			result[i * 4 + 3] = static_cast<uint8_t>(m_state[i]);
		}

		reset();
		m_hash = result;
		return result;
	}

	void SHA256Hash::processBlock()
	{
		std::array<uint32_t, 64> w{};

		for (size_t i = 0; i < 16; ++i) {
			w[i] = (static_cast<uint32_t>(m_buffer[i * 4]) << 24) |
				   (static_cast<uint32_t>(m_buffer[i * 4 + 1]) << 16) |
				   (static_cast<uint32_t>(m_buffer[i * 4 + 2]) << 8) |
				   (static_cast<uint32_t>(m_buffer[i * 4 + 3]));
		}

		for (size_t i = 16; i < 64; ++i) {
			w[i] = sig1(w[i - 2]) + w[i - 7] + sig0(w[i - 15]) + w[i - 16];
		}

		uint32_t a = m_state[0];
		uint32_t b = m_state[1];
		uint32_t c = m_state[2];
		uint32_t d = m_state[3];
		uint32_t e = m_state[4];
		uint32_t f = m_state[5];
		uint32_t g = m_state[6];
		uint32_t h = m_state[7];

		for (size_t i = 0; i < 64; ++i) {
			uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
			uint32_t ch = choose(e, f, g);
			uint32_t temp1 = h + S1 + ch + K[i] + w[i];
			uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
			uint32_t maj = majority(a, b, c);
			uint32_t temp2 = S0 + maj;

			h = g;
			g = f;
			f = e;
			e = d + temp1;
			d = c;
			c = b;
			b = a;
			a = temp1 + temp2;
		}

		// Add compressed chunk to current hash value
		m_state[0] += a;
		m_state[1] += b;
		m_state[2] += c;
		m_state[3] += d;
		m_state[4] += e;
		m_state[5] += f;
		m_state[6] += g;
		m_state[7] += h;
	}

	std::string SHA256Hash::toHexString() const
	{
		return toHexString(false);
	}

	std::string SHA256Hash::toHexString(bool uppercase) const
	{
		std::ostringstream ss;
		ss << std::hex << std::setfill('0');
		if (uppercase) {
			ss << std::uppercase;
		}

		for (uint8_t byte : m_hash) {
			ss << std::setw(2) << static_cast<uint32_t>(byte);
		}

		return ss.str();
	}

	SHA256Hash SHA256Hash::fromHexString(const std::string& hex)
	{
		if (hex.length() != 64) {
			throw std::invalid_argument("SHA-256 hex string must be exactly 64 characters");
		}

		HashValue result{};
		for (size_t i = 0; i < 32; ++i) {
			std::string byteString = hex.substr(i * 2, 2);

			// Validate hex characters
			for (char c : byteString) {
				if (!std::isxdigit(c)) {
					throw std::invalid_argument("Invalid hex character in SHA-256 string");
				}
			}

			result[i] = static_cast<uint8_t>(std::stoul(byteString, nullptr, 16));
		}

		return SHA256Hash(result);
	}

}