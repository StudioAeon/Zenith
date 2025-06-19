#pragma once

#include "AssetSerializer.hpp"

#include "Zenith/Serialization/FileStream.hpp"

namespace Zenith {

	class AssetImporter
	{
	public:
		static void Init();
		static void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset);
		static void Serialize(const Ref<Asset>& asset);
		static bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset);
		static void RegisterDependencies(const AssetMetadata& metadata);

	private:
		static std::unordered_map<AssetType, Scope<AssetSerializer>> s_Serializers;
	};

}