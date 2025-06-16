#pragma once

#include <type_traits>
#include <concepts>
#include <iostream>
#include <vector>
#include <array>
#include <string_view>

namespace Zenith::Reflection::Type {

	//==============================================================================
	// C++20 Concepts for better constraints and error messages
	//==============================================================================

	template<typename T>
	concept Streamable = requires(std::ostream& os, const T& t) {
		os << t;
	};

	template<typename T>
	concept Array = requires {
		typename T::value_type;
		std::size(std::declval<T>());
	} || std::is_array_v<T>;

	template<typename T>
	concept MemberPointer = std::is_member_pointer_v<T>;

	template<typename T>
	concept MemberFunctionPointer = std::is_member_function_pointer_v<T>;

	template<typename T>
	concept MemberObjectPointer = std::is_member_object_pointer_v<T> && !MemberFunctionPointer<T>;

	//==============================================================================
	// Member pointer utilities - optimized and constexpr
	//==============================================================================

	namespace detail {
		template<typename T>
		struct MemberPointerTraits;

		// Member function specializations
		template<typename Class, typename Return, typename... Args>
		struct MemberPointerTraits<Return(Class::*)(Args...)> {
			using return_type = Return;
			using class_type = Class;
			using function_type = Return(Args...);
			static constexpr bool is_const = false;
			static constexpr bool is_function = true;
		};

		template<typename Class, typename Return, typename... Args>
		struct MemberPointerTraits<Return(Class::*)(Args...) const> {
			using return_type = Return;
			using class_type = Class;
			using function_type = Return(Args...);
			static constexpr bool is_const = true;
			static constexpr bool is_function = true;
		};

		template<typename Class, typename Return, typename... Args>
		struct MemberPointerTraits<Return(Class::*)(Args...) noexcept> {
			using return_type = Return;
			using class_type = Class;
			using function_type = Return(Args...);
			static constexpr bool is_const = false;
			static constexpr bool is_function = true;
		};

		template<typename Class, typename Return, typename... Args>
		struct MemberPointerTraits<Return(Class::*)(Args...) const noexcept> {
			using return_type = Return;
			using class_type = Class;
			using function_type = Return(Args...);
			static constexpr bool is_const = true;
			static constexpr bool is_function = true;
		};

		// Member object specialization
		template<typename Class, typename Type>
		struct MemberPointerTraits<Type Class::*> {
			using return_type = Type;
			using class_type = Class;
			using member_type = Type;
			static constexpr bool is_const = false;
			static constexpr bool is_function = false;
		};
	}

	template<MemberPointer T>
	using MemberReturnType = typename detail::MemberPointerTraits<T>::return_type;

	template<MemberPointer T>
	using MemberClassType = typename detail::MemberPointerTraits<T>::class_type;

	template<MemberPointer T>
	constexpr bool IsMemberConst = detail::MemberPointerTraits<T>::is_const;

	template<MemberPointer T>
	constexpr bool IsMemberFunction = detail::MemberPointerTraits<T>::is_function;

	//==============================================================================
	// Type filtering utilities - replace void with alternative
	//==============================================================================

	struct VoidAlternative {};

	template<typename T, typename Alt = VoidAlternative>
	using FilterVoid = std::conditional_t<std::is_void_v<T>, Alt, T>;

	//==============================================================================
	// Container type detection - more comprehensive
	//==============================================================================

	namespace detail {
		template<typename T>
		struct IsStdArray : std::false_type {};

		template<typename T, size_t N>
		struct IsStdArray<std::array<T, N>> : std::true_type {};

		template<typename T>
		struct IsStdVector : std::false_type {};

		template<typename T, typename Alloc>
		struct IsStdVector<std::vector<T, Alloc>> : std::true_type {};
	}

	template<typename T>
	constexpr bool IsStdArray_v = detail::IsStdArray<std::decay_t<T>>::value;

	template<typename T>
	constexpr bool IsStdVector_v = detail::IsStdVector<std::decay_t<T>>::value;

	template<typename T>
	constexpr bool IsArrayLike_v = std::is_array_v<T> || IsStdArray_v<T> || IsStdVector_v<T>;

	//==============================================================================
	// Specialization detection - improved SFINAE
	//==============================================================================

	template<typename T, typename = void>
	struct IsSpecialized : std::false_type {};

	template<typename T>
	struct IsSpecialized<T, std::void_t<decltype(T{})>> : std::true_type {};

	template<typename T>
	constexpr bool IsSpecialized_v = IsSpecialized<T>::value;

	//==============================================================================
	// Tuple utilities - constexpr and noexcept where possible
	//==============================================================================

	template<typename Tuple, typename Func>
	constexpr void ForEachTuple(Tuple&& tuple, Func&& func) noexcept(
		noexcept(std::apply([&func](auto&&... args) {
			(func(std::forward<decltype(args)>(args)), ...);
		}, std::forward<Tuple>(tuple)))
	) {
		std::apply([&func](auto&&... args) {
			(func(std::forward<decltype(args)>(args)), ...);
		}, std::forward<Tuple>(tuple));
	}

	//==============================================================================
	// Compile-time string utilities for member names
	//==============================================================================

	namespace detail {
		constexpr size_t FindChar(std::string_view str, char c, size_t start = 0) {
			for (size_t i = start; i < str.size(); ++i) {
				if (str[i] == c) return i;
			}
			return std::string_view::npos;
		}

		constexpr size_t FindLastChar(std::string_view str, char c) {
			for (size_t i = str.size(); i > 0; --i) {
				if (str[i - 1] == c) return i - 1;
			}
			return std::string_view::npos;
		}

		constexpr std::string_view TrimWhitespace(std::string_view str) {
			while (!str.empty() && std::isspace(str.front())) {
				str.remove_prefix(1);
			}
			while (!str.empty() && std::isspace(str.back())) {
				str.remove_suffix(1);
			}
			return str;
		}

		constexpr std::string_view RemoveNamespace(std::string_view str) {
			auto pos = FindLastChar(str, ':');
			if (pos != std::string_view::npos && pos > 0 && str[pos - 1] == ':') {
				return str.substr(pos + 1);
			}
			return str;
		}

		template<size_t N>
		constexpr std::array<std::string_view, N> SplitString(std::string_view str, std::string_view delimiter) {
			std::array<std::string_view, N> result{};
			size_t count = 0;
			size_t start = 0;

			while (count < N && start < str.size()) {
				auto pos = str.find(delimiter, start);
				if (pos == std::string_view::npos) {
					pos = str.size();
				}

				result[count] = TrimWhitespace(str.substr(start, pos - start));
				++count;
				start = pos + delimiter.size();
			}

			return result;
		}

		template<size_t N>
		constexpr std::array<std::string_view, N> RemoveNamespaces(const std::array<std::string_view, N>& names) {
			std::array<std::string_view, N> result{};
			for (size_t i = 0; i < N; ++i) {
				result[i] = RemoveNamespace(names[i]);
			}
			return result;
		}
	}

	//==============================================================================
	// Type name extraction utilities
	//==============================================================================

	template<typename T>
	constexpr std::string_view GetTypeName() noexcept;

	template<typename T>
	constexpr std::string_view GetTypeNameWithNamespace() noexcept;

	//==============================================================================
	// Alignment and size utilities for reflection
	//==============================================================================

	template<typename T>
	struct TypeInfo {
		static constexpr size_t size = sizeof(T);
		static constexpr size_t alignment = alignof(T);
		static constexpr bool is_trivial = std::is_trivial_v<T>;
		static constexpr bool is_standard_layout = std::is_standard_layout_v<T>;
		static constexpr bool is_pod = std::is_trivial_v<T> && std::is_standard_layout_v<T>;
	};

	//==============================================================================
	// SFINAE helpers for type detection
	//==============================================================================

	template<typename T, typename = void>
	struct HasValueType : std::false_type {};

	template<typename T>
	struct HasValueType<T, std::void_t<typename T::value_type>> : std::true_type {};

	template<typename T>
	constexpr bool HasValueType_v = HasValueType<T>::value;

	template<typename T, typename = void>
	struct HasIterator : std::false_type {};

	template<typename T>
	struct HasIterator<T, std::void_t<typename T::iterator>> : std::true_type {};

	template<typename T>
	constexpr bool HasIterator_v = HasIterator<T>::value;

	//==============================================================================
	// Memory layout utilities for efficient access patterns
	//==============================================================================

	template<typename T>
	constexpr size_t CacheLineSize = 64; // Standard cache line size

	template<typename T>
	constexpr bool IsCacheAligned = alignof(T) >= CacheLineSize<T>;

	// Helper to determine if types should be stored contiguously
	template<typename... Types>
	constexpr bool AreContiguouslyStorable = (std::is_trivially_copyable_v<Types> && ...);

}