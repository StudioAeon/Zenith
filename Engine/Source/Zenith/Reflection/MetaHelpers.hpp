#pragma once

#include <type_traits>
#include <concepts>
#include <utility>

namespace Zenith::Reflection::Meta {

	//==============================================================================
	// C++20 Concepts for better error messages and constraints
	//==============================================================================

	template<typename T>
	concept Type = !std::is_void_v<T>;

	template<typename T>
	concept Complete = requires { sizeof(T); };

	template<template<typename...> class Template, typename T>
	concept TemplateSpecialization = requires {
		[]<typename... Args>(Template<Args...>*) {}(static_cast<T*>(nullptr));
	};

	//==============================================================================
	// Type sequence utilities - more efficient than std::tuple-based approaches
	//==============================================================================

	template<typename...>
	struct TypeList {
		static constexpr size_t size = 0;
	};

	template<typename Head, typename... Tail>
	struct TypeList<Head, Tail...> {
		using head = Head;
		using tail = TypeList<Tail...>;
		static constexpr size_t size = 1 + sizeof...(Tail);
	};

	template<auto...>
	struct ValueList {
		static constexpr size_t size = 0;
	};

	template<auto Head, auto... Tail>
	struct ValueList<Head, Tail...> {
		static constexpr auto head = Head;
		using tail = ValueList<Tail...>;
		static constexpr size_t size = 1 + sizeof...(Tail);
	};

	//==============================================================================
	// Type list operations - all constexpr and noexcept
	//==============================================================================

	// Get element at index
	template<size_t N, typename List>
	struct At;

	template<size_t N, typename Head, typename... Tail>
	struct At<N, TypeList<Head, Tail...>> {
		using type = std::conditional_t<N == 0, Head, typename At<N-1, TypeList<Tail...>>::type>;
	};

	template<size_t N, typename List>
	using At_t = typename At<N, List>::type;

	// Push front
	template<typename T, typename List>
	struct PushFront;

	template<typename T, typename... Types>
	struct PushFront<T, TypeList<Types...>> {
		using type = TypeList<T, Types...>;
	};

	template<typename T, typename List>
	using PushFront_t = typename PushFront<T, List>::type;

	// Pop front
	template<typename List>
	struct PopFront;

	template<typename Head, typename... Tail>
	struct PopFront<TypeList<Head, Tail...>> {
		using type = TypeList<Tail...>;
	};

	template<typename List>
	using PopFront_t = typename PopFront<List>::type;

	// Concatenate lists
	template<typename... Lists>
	struct Concat;

	template<>
	struct Concat<> {
		using type = TypeList<>;
	};

	template<typename... Types>
	struct Concat<TypeList<Types...>> {
		using type = TypeList<Types...>;
	};

	template<typename... Types1, typename... Types2, typename... Rest>
	struct Concat<TypeList<Types1...>, TypeList<Types2...>, Rest...> {
		using type = typename Concat<TypeList<Types1..., Types2...>, Rest...>::type;
	};

	template<typename... Lists>
	using Concat_t = typename Concat<Lists...>::type;

	// Transform each type in list
	template<template<typename> class F, typename List>
	struct Transform;

	template<template<typename> class F, typename... Types>
	struct Transform<F, TypeList<Types...>> {
		using type = TypeList<F<Types>...>;
	};

	template<template<typename> class F, typename List>
	using Transform_t = typename Transform<F, List>::type;

	// Find index of type
	template<typename T, typename List>
	struct FindIndex;

	template<typename T>
	struct FindIndex<T, TypeList<>> {
		static constexpr size_t value = ~size_t{0}; // npos
	};

	template<typename T, typename Head, typename... Tail>
	struct FindIndex<T, TypeList<Head, Tail...>> {
		static constexpr size_t value = std::is_same_v<T, Head>
			? 0
			: (FindIndex<T, TypeList<Tail...>>::value == ~size_t{0}
				? ~size_t{0}
				: 1 + FindIndex<T, TypeList<Tail...>>::value);
	};

	template<typename T, typename List>
	inline constexpr size_t FindIndex_v = FindIndex<T, List>::value;

	inline constexpr size_t npos = ~size_t{0};

	// Check if type exists in list
	template<typename T, typename List>
	inline constexpr bool Contains_v = FindIndex_v<T, List> != npos;

	// Filter list by predicate
	template<template<typename> class Pred, typename List>
	struct Filter;

	template<template<typename> class Pred>
	struct Filter<Pred, TypeList<>> {
		using type = TypeList<>;
	};

	template<template<typename> class Pred, typename Head, typename... Tail>
	struct Filter<Pred, TypeList<Head, Tail...>> {
		using type = std::conditional_t<
			Pred<Head>::value,
			PushFront_t<Head, typename Filter<Pred, TypeList<Tail...>>::type>,
			typename Filter<Pred, TypeList<Tail...>>::type
		>;
	};

	template<template<typename> class Pred, typename List>
	using Filter_t = typename Filter<Pred, List>::type;

	//==============================================================================
	// Functional programming utilities
	//==============================================================================

	template<template<typename...> class F>
	struct Quote {
		template<typename... Args>
		using Invoke = F<Args...>;
	};

	template<typename F, typename... Args>
	using Apply = typename F::template Invoke<Args...>;

	// Bind arguments to the back of a meta-function
	template<typename F, typename... Args>
	struct BindBack {
		template<typename... FrontArgs>
		using Invoke = Apply<F, FrontArgs..., Args...>;
	};

	//==============================================================================
	// Integer sequence utilities
	//==============================================================================

	template<typename List>
	struct AsIntegerSequence;

	template<typename T, T... Values>
	struct AsIntegerSequence<ValueList<Values...>> {
		using type = std::integer_sequence<T, Values...>;
	};

	template<typename List>
	using AsIntegerSequence_t = typename AsIntegerSequence<List>::type;

	//==============================================================================
	// Compile-time string utilities for reflection
	//==============================================================================

	template<size_t N>
	struct FixedString {
		char data[N]{};

		constexpr FixedString() = default;

		constexpr FixedString(const char (&str)[N]) {
			std::copy_n(str, N, data);
		}

		constexpr operator std::string_view() const {
			return {data, N - 1}; // -1 to exclude null terminator
		}

		constexpr size_t size() const { return N - 1; }
		constexpr const char* c_str() const { return data; }

		constexpr bool operator==(const FixedString& other) const {
			return std::string_view{*this} == std::string_view{other};
		}
	};

	// Deduction guide
	template<size_t N>
	FixedString(const char (&)[N]) -> FixedString<N>;

	//==============================================================================
	// Type validation utilities
	//==============================================================================

	template<typename T>
	struct RemoveCVRef {
		using type = std::remove_cvref_t<T>;
	};

	template<typename T>
	using RemoveCVRef_t = typename RemoveCVRef<T>::type;

	// Check if type is specialized
	template<typename T, typename = void>
	struct IsSpecialized : std::false_type {};

	template<typename T>
	struct IsSpecialized<T, std::void_t<decltype(T{})>> : std::true_type {};

	template<typename T>
	inline constexpr bool IsSpecialized_v = IsSpecialized<T>::value;

	// Check if all types are the same
	template<typename... Types>
	inline constexpr bool AllSame_v = (std::is_same_v<Types, typename TypeList<Types...>::head> && ...);

	// Check if all types are convertible to T
	template<typename To, typename... From>
	inline constexpr bool AllConvertibleTo_v = (std::is_convertible_v<From, To> && ...);

	//==============================================================================
	// Nth element access (C++20 optimized)
	//==============================================================================

	template<size_t N, typename... Args>
	constexpr decltype(auto) nth_element(Args&&... args) noexcept {
		static_assert(N < sizeof...(Args), "Index out of bounds");

		return [&]<size_t... Is>(std::index_sequence<Is...>) -> decltype(auto) {
			return [](decltype((void*)Is)..., auto&& nth, auto&&...) -> decltype(auto) {
				return std::forward<decltype(nth)>(nth);
			}(std::addressof(args)...);
		}(std::make_index_sequence<N>{});
	}

}