#pragma once

#include "TypeName.hpp"
#include "TypeUtils.hpp"
#include "MetaHelpers.hpp"

#include <type_traits>
#include <concepts>
#include <string_view>
#include <variant>
#include <optional>
#include <array>
#include <tuple>

namespace Zenith::Reflection {

	//==============================================================================
	// Forward declarations and core concepts
	//==============================================================================

	struct ReflectionTag {};

	template<typename T, typename Tag = ReflectionTag>
	struct TypeDescriptor;

	template<typename T, typename Tag = ReflectionTag>
	concept Described = Type::IsSpecialized_v<TypeDescriptor<std::remove_cvref_t<T>, Tag>>;

	template<typename T>
	concept MemberPointer = Type::MemberPointer<T>;

	template<typename T>
	concept StreamableType = Type::Streamable<T>;

	//==============================================================================
	// Compile-time string utilities for member name parsing
	//==============================================================================

	namespace detail {
		template<size_t N>
		constexpr std::array<std::string_view, N> ParseMemberNames(std::string_view member_str) {
			constexpr std::string_view delimiter = ", ";
			std::array<std::string_view, N> result{};

			size_t start = 0;
			for (size_t i = 0; i < N && start < member_str.size(); ++i) {
				auto end = member_str.find(delimiter, start);
				if (end == std::string_view::npos) {
					end = member_str.size();
				}

				auto member_name = member_str.substr(start, end - start);
				// Remove whitespace and namespace prefixes
				while (!member_name.empty() && std::isspace(member_name.front())) {
					member_name.remove_prefix(1);
				}
				while (!member_name.empty() && std::isspace(member_name.back())) {
					member_name.remove_suffix(1);
				}

				// Remove &ClassName:: prefix if present
				auto pos = member_name.find("::");
				if (pos != std::string_view::npos) {
					member_name = member_name.substr(pos + 2);
				}

				result[i] = member_name;
				start = end + delimiter.size();
			}

			return result;
		}
	}

	//==============================================================================
	// Enhanced member list with modern C++20 features
	//==============================================================================

	template<auto... MemberPointers>
	class MemberList {
	public:
		static constexpr size_t size = sizeof...(MemberPointers);

		// Type aliases for better readability
		using MemberTuple = std::tuple<decltype(MemberPointers)...>;

	private:
		template<size_t Index>
		using MemberType = Type::FilterVoid<Type::MemberReturnType<std::tuple_element_t<Index, MemberTuple>>>;

		template<typename Ptr>
		using MemberPtrType = Type::FilterVoid<Type::MemberReturnType<Ptr>>;

	public:
		using VariantType = std::variant<MemberPtrType<decltype(MemberPointers)>...>;

		//==============================================================================
		// Compile-time member access
		//==============================================================================

		template<size_t Index>
		static constexpr auto GetMember() noexcept {
			static_assert(Index < size, "Member index out of bounds");
			return std::get<Index>(std::tuple{MemberPointers...});
		}

		template<size_t Index>
		static constexpr bool IsFunction() noexcept {
			static_assert(Index < size, "Member index out of bounds");
			return Type::IsMemberFunction<decltype(GetMember<Index>())>;
		}

		template<size_t Index>
		static constexpr size_t GetMemberSize() noexcept {
			static_assert(Index < size, "Member index out of bounds");
			if constexpr (IsFunction<Index>()) {
				return 0;
			} else {
				return sizeof(MemberType<Index>);
			}
		}

		template<size_t Index>
		static constexpr std::string_view GetTypeName() noexcept {
			static_assert(Index < size, "Member index out of bounds");
			return Type::GetTypeName<MemberType<Index>>();
		}

		//==============================================================================
		// Runtime member access with bounds checking
		//==============================================================================

		static std::optional<bool> IsFunction(size_t index) noexcept {
			if (index >= size) return std::nullopt;

			return [index]<size_t... Is>(std::index_sequence<Is...>) -> bool {
				bool result = false;
				((Is == index ? (result = IsFunction<Is>(), true) : false) || ...);
				return result;
			}(std::make_index_sequence<size>{});
		}

		static std::optional<size_t> GetMemberSize(size_t index) noexcept {
			if (index >= size) return std::nullopt;

			return [index]<size_t... Is>(std::index_sequence<Is...>) -> size_t {
				size_t result = 0;
				((Is == index ? (result = GetMemberSize<Is>(), true) : false) || ...);
				return result;
			}(std::make_index_sequence<size>{});
		}

		static std::optional<std::string_view> GetTypeName(size_t index) noexcept {
			if (index >= size) return std::nullopt;

			return [index]<size_t... Is>(std::index_sequence<Is...>) -> std::string_view {
				std::string_view result{};
				((Is == index ? (result = GetTypeName<Is>(), true) : false) || ...);
				return result;
			}(std::make_index_sequence<size>{});
		}

		//==============================================================================
		// Member value access with perfect forwarding and type safety
		//==============================================================================

		template<size_t Index, typename Object>
		static constexpr decltype(auto) GetMemberValue(Object&& obj)
			noexcept(noexcept(std::forward<Object>(obj).*GetMember<Index>()))
		{
			static_assert(Index < size, "Member index out of bounds");

			if constexpr (IsFunction<Index>()) {
				// Return a callable for member functions
				return [&obj](auto&&... args) -> decltype(auto) {
					return (obj.*GetMember<Index>())(std::forward<decltype(args)>(args)...);
				};
			} else {
				return std::forward<Object>(obj).*GetMember<Index>();
			}
		}

		template<size_t Index, typename Value, typename Object>
		static constexpr bool SetMemberValue(Object&& obj, Value&& value)
			noexcept(noexcept(std::forward<Object>(obj).*GetMember<Index>() = std::forward<Value>(value)))
		{
			static_assert(Index < size, "Member index out of bounds");

			if constexpr (IsFunction<Index>()) {
				return false; // Cannot set function members
			} else {
				using TargetType = std::remove_reference_t<decltype(obj.*GetMember<Index>())>;
				using ValueType = std::remove_cvref_t<Value>;

				if constexpr (std::is_assignable_v<TargetType&, Value>) {
					std::forward<Object>(obj).*GetMember<Index>() = std::forward<Value>(value);
					return true;
				}
				return false;
			}
		}

		//==============================================================================
		// Variadic operations on all members
		//==============================================================================

		template<typename Object, typename Func>
		static constexpr void ForEachMember(Object&& obj, Func&& func) {
			[&]<size_t... Is>(std::index_sequence<Is...>) {
				(ForEachMemberImpl<Is>(std::forward<Object>(obj), std::forward<Func>(func)), ...);
			}(std::make_index_sequence<size>{});
		}

		template<typename Func>
		static constexpr void ForEachMemberType(Func&& func) {
			[&]<size_t... Is>(std::index_sequence<Is...>) {
				(func.template operator()<MemberType<Is>, Is>(), ...);
			}(std::make_index_sequence<size>{});
		}

	private:
		template<size_t Index, typename Object, typename Func>
		static constexpr void ForEachMemberImpl(Object&& obj, Func&& func) {
			if constexpr (!IsFunction<Index>()) {
				func(GetMemberValue<Index>(std::forward<Object>(obj)));
			}
		}
	};

	//==============================================================================
	// Type descriptor interface with compile-time optimization
	//==============================================================================

	template<typename DescriptorType, typename ObjectType, typename Tag, typename MemberListType>
	class TypeDescriptorInterface {
	public:
		static constexpr size_t member_count = MemberListType::size;
		static constexpr size_t invalid_index = SIZE_MAX;

		//==============================================================================
		// Member name lookup with compile-time optimization
		//==============================================================================

		static constexpr size_t IndexOf(std::string_view member_name) noexcept {
			const auto& names = DescriptorType::member_names;
			for (size_t i = 0; i < member_count; ++i) {
				if (names[i] == member_name) {
					return i;
				}
			}
			return invalid_index;
		}

		static constexpr bool HasMember(std::string_view member_name) noexcept {
			return IndexOf(member_name) != invalid_index;
		}

		static constexpr std::optional<std::string_view> GetMemberName(size_t index) noexcept {
			if (index >= member_count) return std::nullopt;
			return DescriptorType::member_names[index];
		}

		template<size_t Index>
		static constexpr std::string_view GetMemberName() noexcept {
			static_assert(Index < member_count, "Member index out of bounds");
			return DescriptorType::member_names[Index];
		}

		//==============================================================================
		// Type-safe member value access by name
		//==============================================================================

		template<typename ValueType, typename Object>
		static std::optional<ValueType> GetMemberValue(std::string_view member_name, Object&& obj) {
			const size_t index = IndexOf(member_name);
			if (index == invalid_index) return std::nullopt;

			return GetMemberValueByIndex<ValueType>(index, std::forward<Object>(obj));
		}

		template<typename ValueType, typename Object>
		static bool SetMemberValue(std::string_view member_name, Object&& obj, ValueType&& value) {
			const size_t index = IndexOf(member_name);
			if (index == invalid_index) return false;

			return SetMemberValueByIndex(index, std::forward<Object>(obj), std::forward<ValueType>(value));
		}

		//==============================================================================
		// Variant-based access for runtime polymorphism
		//==============================================================================

		template<typename Object>
		static typename MemberListType::VariantType GetMemberVariant(size_t index, Object&& obj) {
			if (index >= member_count) {
				return typename MemberListType::VariantType{};
			}

			return [&]<size_t... Is>(std::index_sequence<Is...>) -> typename MemberListType::VariantType {
				typename MemberListType::VariantType result{};
				((Is == index ? (result = MemberListType::template GetMemberValue<Is>(std::forward<Object>(obj)), true) : false) || ...);
				return result;
			}(std::make_index_sequence<member_count>{});
		}

		//==============================================================================
		// Debug and introspection utilities
		//==============================================================================

		template<StreamableType Stream>
		static void PrintTypeInfo(Stream& stream) {
			stream << "Type: " << DescriptorType::class_name << '\n';
			stream << "Namespace: " << DescriptorType::namespace_name << '\n';
			stream << "Size: " << sizeof(ObjectType) << " bytes\n";
			stream << "Alignment: " << alignof(ObjectType) << " bytes\n";
			stream << "Members (" << member_count << "):\n";

			for (size_t i = 0; i < member_count; ++i) {
				stream << "  [" << i << "] ";
				if (auto type_name = MemberListType::GetTypeName(i)) {
					stream << *type_name;
				}
				stream << " " << DescriptorType::member_names[i];

				if (auto size = MemberListType::GetMemberSize(i)) {
					if (*size == 0) {
						stream << " (function)";
					} else {
						stream << " (" << *size << " bytes)";
					}
				}
				stream << '\n';
			}
		}

		template<StreamableType Stream, typename Object>
		static void PrintObjectInfo(Stream& stream, const Object& obj) {
			PrintTypeInfo(stream);
			stream << "Values:\n";

			MemberListType::ForEachMember(obj, [&, i = 0](const auto& member) mutable {
				stream << "  " << DescriptorType::member_names[i] << " = ";
				if constexpr (StreamableType<std::remove_cvref_t<decltype(member)>>) {
					stream << member;
				} else {
					stream << "<non-streamable>";
				}
				stream << '\n';
				++i;
			});
		}

	private:
		template<typename ValueType, typename Object>
		static std::optional<ValueType> GetMemberValueByIndex(size_t index, Object&& obj) {
			return [&]<size_t... Is>(std::index_sequence<Is...>) -> std::optional<ValueType> {
				std::optional<ValueType> result{};
				((Is == index ? TryGetMemberValue<Is, ValueType>(result, std::forward<Object>(obj)) : false) || ...);
				return result;
			}(std::make_index_sequence<member_count>{});
		}

		template<size_t Index, typename ValueType, typename Object>
		static bool TryGetMemberValue(std::optional<ValueType>& result, Object&& obj) {
			if constexpr (!MemberListType::template IsFunction<Index>()) {
				using MemberType = std::remove_cvref_t<decltype(MemberListType::template GetMemberValue<Index>(obj))>;
				if constexpr (std::is_same_v<ValueType, MemberType> || std::is_convertible_v<MemberType, ValueType>) {
					result = static_cast<ValueType>(MemberListType::template GetMemberValue<Index>(std::forward<Object>(obj)));
					return true;
				}
			}
			return false;
		}

		template<typename ValueType, typename Object>
		static bool SetMemberValueByIndex(size_t index, Object&& obj, ValueType&& value) {
			return [&]<size_t... Is>(std::index_sequence<Is...>) -> bool {
				return ((Is == index ? MemberListType::template SetMemberValue<Is>(std::forward<Object>(obj), std::forward<ValueType>(value)) : false) || ...);
			}(std::make_index_sequence<member_count>{});
		}
	};

	//==============================================================================
	// Modern macro for type registration with better error handling
	//==============================================================================

#define ZENITH_REFLECT_TYPE_TAGGED(Class, Tag, ...)						\
	template<>																\
	struct Zenith::Reflection::TypeDescriptor<Class, Tag> 					\
		: public Zenith::Reflection::MemberList<__VA_ARGS__>,				\
		  public Zenith::Reflection::TypeDescriptorInterface<				\
		  	Zenith::Reflection::TypeDescriptor<Class, Tag>,				\
		  	Class,															\
		  	Tag,															\
		  	Zenith::Reflection::MemberList<__VA_ARGS__>						\
		  > 																\
	{																		\
	private:																\
		static constexpr std::string_view member_string{ #__VA_ARGS__ };	\
		static constexpr std::string_view class_string{ #Class };			\
																			\
	public:																	\
		static constexpr std::string_view class_name = 					\
			Zenith::Reflection::Type::StripNamespace(class_string);		\
		static constexpr std::string_view namespace_name = 				\
			Zenith::Reflection::Type::ExtractNamespace(class_string);		\
		static constexpr std::array<std::string_view, member_count> 		\
			member_names = Zenith::Reflection::detail::ParseMemberNames<member_count>(member_string); \
																			\
		static constexpr uint64_t type_hash = 								\
			Zenith::Reflection::Type::GetTypeHash<Class>();				\
	}

#define ZENITH_REFLECT_TYPE(Class, ...) 									\
	ZENITH_REFLECT_TYPE_TAGGED(Class, Zenith::Reflection::ReflectionTag, __VA_ARGS__)

	// Backwards compatibility macros
#define DESCRIBED_TAGGED(Class, Tag, ...) ZENITH_REFLECT_TYPE_TAGGED(Class, Tag, __VA_ARGS__)
#define DESCRIBED(Class, ...) ZENITH_REFLECT_TYPE(Class, __VA_ARGS__)

} // namespace Zenith::Reflection