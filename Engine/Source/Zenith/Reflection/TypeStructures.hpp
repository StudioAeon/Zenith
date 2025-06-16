#pragma once

#include "TypeDescriptor.hpp"
#include "TypeName.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <concepts>
#include <span>

namespace Zenith::Reflection {

	//==============================================================================
	// Enum for member types with better type safety
	//==============================================================================

	enum class MemberKind : uint8_t {
		DataMember,
		MemberFunction,
		Unknown
	};

	//==============================================================================
	// Enhanced member information structure
	//==============================================================================

	struct MemberInfo {
		std::string name;
		std::string type_name;
		size_t size_bytes = 0;
		size_t alignment = 0;
		MemberKind kind = MemberKind::Unknown;

		// Additional metadata
		bool is_const = false;
		bool is_reference = false;
		bool is_pointer = false;
		bool is_trivial = false;

		constexpr bool operator==(const MemberInfo& other) const noexcept {
			return name == other.name &&
				   type_name == other.type_name &&
				   size_bytes == other.size_bytes &&
				   alignment == other.alignment &&
				   kind == other.kind &&
				   is_const == other.is_const &&
				   is_reference == other.is_reference &&
				   is_pointer == other.is_pointer &&
				   is_trivial == other.is_trivial;
		}
	};

	//==============================================================================
	// Comprehensive class information with caching
	//==============================================================================

	class ClassInfo {
	public:
		std::string name;
		std::string namespace_name;
		std::string full_name;
		uint64_t type_hash = 0;

		size_t size_bytes = 0;
		size_t alignment = 0;

		std::vector<MemberInfo> members;

		// Type traits
		bool is_trivial = false;
		bool is_standard_layout = false;
		bool is_pod = false;
		bool is_polymorphic = false;

		//==============================================================================
		// Factory method to create ClassInfo from reflected types
		//==============================================================================

		template<Described T>
		static ClassInfo Create() {
			using Descriptor = TypeDescriptor<std::remove_cvref_t<T>>;

			ClassInfo info;
			info.name = std::string{Descriptor::class_name};
			info.namespace_name = std::string{Descriptor::namespace_name};
			info.full_name = info.namespace_name.empty() ? info.name : info.namespace_name + "::" + info.name;
			info.type_hash = Descriptor::type_hash;

			info.size_bytes = sizeof(T);
			info.alignment = alignof(T);

			info.is_trivial = std::is_trivial_v<T>;
			info.is_standard_layout = std::is_standard_layout_v<T>;
			info.is_pod = info.is_trivial && info.is_standard_layout;
			info.is_polymorphic = std::is_polymorphic_v<T>;

			// Parse member information
			info.members.reserve(Descriptor::member_count);

			for (size_t i = 0; i < Descriptor::member_count; ++i) {
				MemberInfo member_info;
				member_info.name = std::string{*Descriptor::GetMemberName(i)};

				if (auto type_name = Descriptor::GetTypeName(i)) {
					member_info.type_name = std::string{*type_name};
				}

				if (auto size = Descriptor::GetMemberSize(i)) {
					member_info.size_bytes = *size;
					member_info.kind = (*size == 0) ? MemberKind::MemberFunction : MemberKind::DataMember;
				}

				// Additional type analysis could be added here
				member_info.alignment = std::min(member_info.size_bytes, alignof(std::max_align_t));

				info.members.emplace_back(std::move(member_info));
			}

			return info;
		}

		//==============================================================================
		// Query methods
		//==============================================================================

		[[nodiscard]] const MemberInfo* FindMember(std::string_view name) const noexcept {
			for (const auto& member : members) {
				if (member.name == name) {
					return &member;
				}
			}
			return nullptr;
		}

		[[nodiscard]] std::span<const MemberInfo> GetDataMembers() const noexcept {
			// This is a simplified implementation - in a real system you'd cache this
			static thread_local std::vector<const MemberInfo*> data_members;
			data_members.clear();

			for (const auto& member : members) {
				if (member.kind == MemberKind::DataMember) {
					data_members.push_back(&member);
				}
			}

			return std::span<const MemberInfo>{reinterpret_cast<const MemberInfo*>(data_members.data()), data_members.size()};
		}

		[[nodiscard]] std::span<const MemberInfo> GetMemberFunctions() const noexcept {
			static thread_local std::vector<const MemberInfo*> functions;
			functions.clear();

			for (const auto& member : members) {
				if (member.kind == MemberKind::MemberFunction) {
					functions.push_back(&member);
				}
			}

			return std::span<const MemberInfo>{reinterpret_cast<const MemberInfo*>(functions.data()), functions.size()};
		}

		[[nodiscard]] size_t GetDataMemberCount() const noexcept {
			return std::count_if(members.begin(), members.end(),
				[](const MemberInfo& m) { return m.kind == MemberKind::DataMember; });
		}

		[[nodiscard]] size_t GetMemberFunctionCount() const noexcept {
			return std::count_if(members.begin(), members.end(),
				[](const MemberInfo& m) { return m.kind == MemberKind::MemberFunction; });
		}

		constexpr bool operator==(const ClassInfo& other) const noexcept {
			return type_hash == other.type_hash &&
				   name == other.name &&
				   namespace_name == other.namespace_name &&
				   size_bytes == other.size_bytes &&
				   alignment == other.alignment &&
				   members == other.members;
		}
	};

	//==============================================================================
	// Type registry for efficient type lookup and management
	//==============================================================================

	class TypeRegistry {
	public:
		using TypeId = uint64_t;
		using CreateInfoFunc = std::function<ClassInfo()>;

	private:
		std::unordered_map<TypeId, ClassInfo> m_TypeInfoCache;
		std::unordered_map<std::string, TypeId> m_NameToIdMap;
		std::unordered_map<TypeId, CreateInfoFunc> m_TypeFactories;

	public:
		//==============================================================================
		// Type registration
		//==============================================================================

		template<Described T>
		void RegisterType() {
			using CleanType = std::remove_cvref_t<T>;
			constexpr TypeId type_id = Type::GetTypeHash<CleanType>();

			// Register factory function
			m_TypeFactories[type_id] = []() -> ClassInfo {
				return ClassInfo::Create<CleanType>();
			};

			// Register name mapping
			auto class_name = std::string{Type::GetTypeName<CleanType>()};
			auto full_name = std::string{Type::GetTypeNameWithNamespace<CleanType>()};

			m_NameToIdMap[class_name] = type_id;
			m_NameToIdMap[full_name] = type_id;
		}

		//==============================================================================
		// Type lookup
		//==============================================================================

		template<typename T>
		[[nodiscard]] const ClassInfo* GetClassInfo() {
			using CleanType = std::remove_cvref_t<T>;
			constexpr TypeId type_id = Type::GetTypeHash<CleanType>();

			return GetClassInfo(type_id);
		}

		[[nodiscard]] const ClassInfo* GetClassInfo(TypeId type_id) {
			auto it = m_TypeInfoCache.find(type_id);
			if (it != m_TypeInfoCache.end()) {
				return &it->second;
			}

			// Try to create info if factory exists
			auto factory_it = m_TypeFactories.find(type_id);
			if (factory_it != m_TypeFactories.end()) {
				auto [inserted_it, success] = m_TypeInfoCache.emplace(type_id, factory_it->second());
				return success ? &inserted_it->second : nullptr;
			}

			return nullptr;
		}

		[[nodiscard]] const ClassInfo* GetClassInfo(std::string_view type_name) {
			auto it = m_NameToIdMap.find(std::string{type_name});
			if (it != m_NameToIdMap.end()) {
				return GetClassInfo(it->second);
			}
			return nullptr;
		}

		//==============================================================================
		// Utility methods
		//==============================================================================

		[[nodiscard]] std::vector<const ClassInfo*> GetAllRegisteredTypes() const {
			std::vector<const ClassInfo*> result;
			result.reserve(m_TypeInfoCache.size());

			for (const auto& [id, info] : m_TypeInfoCache) {
				result.push_back(&info);
			}

			return result;
		}

		[[nodiscard]] size_t GetRegisteredTypeCount() const noexcept {
			return m_TypeFactories.size();
		}

		void ClearCache() {
			m_TypeInfoCache.clear();
		}

		template<typename T>
		[[nodiscard]] bool IsRegistered() const noexcept {
			using CleanType = std::remove_cvref_t<T>;
			constexpr TypeId type_id = Type::GetTypeHash<CleanType>();
			return m_TypeFactories.contains(type_id);
		}

		//==============================================================================
		// Singleton access
		//==============================================================================

		static TypeRegistry& Instance() {
			static TypeRegistry instance;
			return instance;
		}
	};

	//==============================================================================
	// Convenience functions for global access
	//==============================================================================

	template<Described T>
	void RegisterType() {
		TypeRegistry::Instance().RegisterType<T>();
	}

	template<typename T>
	const ClassInfo* GetClassInfo() {
		return TypeRegistry::Instance().GetClassInfo<T>();
	}

	inline const ClassInfo* GetClassInfo(std::string_view type_name) {
		return TypeRegistry::Instance().GetClassInfo(type_name);
	}

	template<typename T>
	bool IsTypeRegistered() {
		return TypeRegistry::Instance().IsRegistered<T>();
	}

}