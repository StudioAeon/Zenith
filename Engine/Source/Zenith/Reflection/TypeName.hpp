#pragma once

#include <string_view>
#include <array>
#include <utility>

namespace Zenith::Reflection::Type {

	//==============================================================================
	// Compile-time type name extraction using compiler intrinsics
	// Optimized for minimal compile-time overhead and runtime performance
	//==============================================================================

	namespace detail {
		// Cache-friendly storage for type names
		template<typename T, bool KeepNamespace>
		struct TypeNameStorage {
			static constexpr auto value = ExtractTypeName<T, KeepNamespace>();
		};

		template<std::size_t... Indices>
		constexpr auto SubstringAsArray(std::string_view str, std::index_sequence<Indices...>) noexcept {
			return std::array<char, sizeof...(Indices) + 1>{{ str[Indices]..., '\0' }};
		}

		template<bool KeepNamespace, typename T>
		constexpr auto ExtractTypeName() noexcept {
			// Use compiler-specific function signature introspection
#if defined(__clang__)
			constexpr std::string_view prefix = "T = ";
			constexpr std::string_view suffix = "]";
			constexpr std::string_view function = __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
			constexpr std::string_view prefix = "with T = ";
			constexpr std::string_view suffix = "]";
			constexpr std::string_view function = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
			constexpr std::string_view prefix = "ExtractTypeName<";
			constexpr std::string_view suffix = ">(void)";
			constexpr std::string_view function = __FUNCSIG__;
#else
#   error "Unsupported compiler for type name extraction"
#endif

			// Find the type name boundaries
			constexpr auto start = function.find(prefix);
			static_assert(start != std::string_view::npos, "Failed to find type name start");

			constexpr auto name_start = start + prefix.size();
			constexpr auto end = function.find(suffix, name_start);
			static_assert(end != std::string_view::npos, "Failed to find type name end");

			constexpr auto full_name = function.substr(name_start, end - name_start);

			if constexpr (KeepNamespace) {
				return SubstringAsArray(full_name, std::make_index_sequence<full_name.size()>{});
			} else {
				// Strip namespace - find last "::" occurrence
				constexpr auto namespace_pos = full_name.rfind("::");
				if constexpr (namespace_pos != std::string_view::npos) {
					constexpr auto stripped_name = full_name.substr(namespace_pos + 2);
					return SubstringAsArray(stripped_name, std::make_index_sequence<stripped_name.size()>{});
				} else {
					return SubstringAsArray(full_name, std::make_index_sequence<full_name.size()>{});
				}
			}
		}
	}

	//==============================================================================
	// Public interface - highly optimized for both compile-time and runtime
	//==============================================================================

	/**
	 * @brief Get type name at compile-time without namespace
	 * @tparam T Type to get name for
	 * @return Compile-time string view of type name
	 */
	template<typename T>
	constexpr std::string_view GetTypeName() noexcept {
		constexpr auto& storage = detail::TypeNameStorage<T, false>::value;
		return std::string_view{ storage.data(), storage.size() - 1 };
	}

	/**
	 * @brief Get type name at compile-time with full namespace
	 * @tparam T Type to get name for
	 * @return Compile-time string view of type name with namespace
	 */
	template<typename T>
	constexpr std::string_view GetTypeNameWithNamespace() noexcept {
		constexpr auto& storage = detail::TypeNameStorage<T, true>::value;
		return std::string_view{ storage.data(), storage.size() - 1 };
	}

	/**
	 * @brief Get clean type name by removing cv-qualifiers and references
	 * @tparam T Type to get clean name for
	 * @return Clean type name without qualifiers
	 */
	template<typename T>
	constexpr std::string_view GetCleanTypeName() noexcept {
		using CleanType = std::remove_cvref_t<T>;
		return GetTypeName<CleanType>();
	}

	/**
	 * @brief Get clean type name with namespace by removing cv-qualifiers and references
	 * @tparam T Type to get clean name for
	 * @return Clean type name with namespace, without qualifiers
	 */
	template<typename T>
	constexpr std::string_view GetCleanTypeNameWithNamespace() noexcept {
		using CleanType = std::remove_cvref_t<T>;
		return GetTypeNameWithNamespace<CleanType>();
	}

	//==============================================================================
	// Utility functions for namespace manipulation
	//==============================================================================

	namespace detail {
		constexpr std::string_view StripNamespace(std::string_view type_name) noexcept {
			auto pos = type_name.rfind("::");
			return (pos != std::string_view::npos) ? type_name.substr(pos + 2) : type_name;
		}

		constexpr std::string_view ExtractNamespace(std::string_view type_name) noexcept {
			auto pos = type_name.rfind("::");
			return (pos != std::string_view::npos) ? type_name.substr(0, pos) : std::string_view{};
		}
	}

	/**
	 * @brief Strip namespace from a type name string
	 * @param type_name Full type name with namespace
	 * @return Type name without namespace
	 */
	constexpr std::string_view StripNamespace(std::string_view type_name) noexcept {
		return detail::StripNamespace(type_name);
	}

	/**
	 * @brief Extract namespace from a type name string
	 * @param type_name Full type name with namespace
	 * @return Namespace part only, empty if no namespace
	 */
	constexpr std::string_view ExtractNamespace(std::string_view type_name) noexcept {
		return detail::ExtractNamespace(type_name);
	}

	//==============================================================================
	// Type name hashing for fast lookups
	//==============================================================================

	namespace detail {
		// FNV-1a hash implementation for compile-time string hashing
		constexpr uint64_t FNV1a_Hash64(std::string_view str) noexcept {
			constexpr uint64_t FNV_offset_basis = 14695981039346656037ULL;
			constexpr uint64_t FNV_prime = 1099511628211ULL;

			uint64_t hash = FNV_offset_basis;
			for (char c : str) {
				hash ^= static_cast<uint64_t>(c);
				hash *= FNV_prime;
			}
			return hash;
		}
	}

	/**
	 * @brief Get compile-time hash of type name for fast comparisons
	 * @tparam T Type to hash
	 * @return Compile-time hash value
	 */
	template<typename T>
	constexpr uint64_t GetTypeHash() noexcept {
		return detail::FNV1a_Hash64(GetTypeName<T>());
	}

	/**
	 * @brief Get compile-time hash of type name with namespace
	 * @tparam T Type to hash
	 * @return Compile-time hash value including namespace
	 */
	template<typename T>
	constexpr uint64_t GetTypeHashWithNamespace() noexcept {
		return detail::FNV1a_Hash64(GetTypeNameWithNamespace<T>());
	}

	//==============================================================================
	// Type name comparison utilities
	//==============================================================================

	/**
	 * @brief Compare two type names for equality
	 * @tparam T1 First type
	 * @tparam T2 Second type
	 * @return true if type names are equal
	 */
	template<typename T1, typename T2>
	constexpr bool TypeNamesEqual() noexcept {
		return GetTypeName<T1>() == GetTypeName<T2>();
	}

	/**
	 * @brief Check if type name starts with given prefix
	 * @tparam T Type to check
	 * @param prefix Prefix to look for
	 * @return true if type name starts with prefix
	 */
	template<typename T>
	constexpr bool TypeNameStartsWith(std::string_view prefix) noexcept {
		auto name = GetTypeName<T>();
		return name.size() >= prefix.size() && name.substr(0, prefix.size()) == prefix;
	}

	/**
	 * @brief Check if type name ends with given suffix
	 * @tparam T Type to check
	 * @param suffix Suffix to look for
	 * @return true if type name ends with suffix
	 */
	template<typename T>
	constexpr bool TypeNameEndsWith(std::string_view suffix) noexcept {
		auto name = GetTypeName<T>();
		return name.size() >= suffix.size() &&
			   name.substr(name.size() - suffix.size()) == suffix;
	}

} // namespace Zenith::Reflection::Type

//==============================================================================
// Backwards compatibility aliases (can be removed in future versions)
//==============================================================================

namespace type {
	template<typename T>
	constexpr auto type_name() -> std::string_view {
		return Zenith::Reflection::Type::GetTypeName<T>();
	}

	template<typename T>
	constexpr auto type_name_keep_namespace() -> std::string_view {
		return Zenith::Reflection::Type::GetTypeNameWithNamespace<T>();
	}
}