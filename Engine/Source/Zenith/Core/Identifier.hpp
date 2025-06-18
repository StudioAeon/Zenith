#pragma once

#include "Hash.hpp"

#include <stdint.h>
#include <cstring>
#include <string>

namespace Zenith
{
	class Identifier
	{
	public:
		constexpr Identifier() {}

		constexpr Identifier(std::string_view name) noexcept
			: hash(Hash::GenerateFNVHash(name)), dbgName(name)
		{
		}

		constexpr Identifier(uint32_t hash) noexcept
			: hash(hash)
		{
		}

		constexpr bool operator==(const Identifier& other) const noexcept { return hash == other.hash; }
		constexpr bool operator!=(const Identifier& other) const noexcept { return hash != other.hash; }

		constexpr operator uint32_t() const noexcept { return hash; }
		constexpr std::string_view GetDBGName() const { return dbgName; }

	private:
		friend struct std::hash<Identifier>;
		uint32_t hash = 0;
		std::string_view dbgName;
	};

} // namespace Zenith


namespace std
{
	template<>
	struct hash<Zenith::Identifier>
	{
		size_t operator()(const Zenith::Identifier& id) const noexcept
		{
			static_assert(noexcept(hash<uint32_t>()(id.hash)), "hash function should not throw");
			return id.hash;
		}
	};
}