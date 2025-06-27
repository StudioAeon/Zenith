#include "znpch.hpp"
#include "AssetImporter.hpp"
#include "AssetManager.hpp"
#include "MeshSerializer.hpp"

#include "Zenith/Debug/Profiler.hpp"

namespace Zenith {

	void AssetImporter::Init()
	{
		s_Serializers.clear();
		s_Serializers[AssetType::Texture] = CreateScope<TextureSerializer>();
		s_Serializers[AssetType::StaticMesh] = CreateScope<StaticMeshSerializer>();
		s_Serializers[AssetType::MeshSource] = CreateScope<MeshSourceSerializer>();
		s_Serializers[AssetType::Material] = CreateScope<MaterialAssetSerializer>();
		s_Serializers[AssetType::Font] = CreateScope<FontSerializer>();
	}

	void AssetImporter::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset)
	{
		if (s_Serializers.find(metadata.Type) == s_Serializers.end())
		{
			ZN_CORE_WARN("There's currently no importer for assets of type {0}", metadata.FilePath.stem().string());
			return;
		}

		s_Serializers[asset->GetAssetType()]->Serialize(metadata, asset);
	}

	void AssetImporter::Serialize(const Ref<Asset>& asset)
	{
		const AssetMetadata& metadata = Project::GetEditorAssetManager()->GetMetadata(asset->Handle);
		Serialize(metadata, asset);
	}

	bool AssetImporter::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset)
	{
		ZN_PROFILE_FUNC("AssetImporter::TryLoadData");

		if (s_Serializers.find(metadata.Type) == s_Serializers.end())
		{
			ZN_CORE_WARN("There's currently no importer for assets of type {0}", metadata.FilePath.stem().string());
			return false;
		}

		// ZN_CORE_TRACE("AssetImporter::TryLoadData - {}", metadata.FilePath);
		return s_Serializers[metadata.Type]->TryLoadData(metadata, asset);
	}

	void AssetImporter::RegisterDependencies(const AssetMetadata& metadata)
	{
		if (s_Serializers.find(metadata.Type) == s_Serializers.end())
		{
			ZN_CORE_WARN("There's currently no importer for assets of type {0}", metadata.FilePath.stem().string());
			return;
		}

		s_Serializers[metadata.Type]->RegisterDependencies(metadata);
	}

	std::unordered_map<AssetType, Scope<AssetSerializer>> AssetImporter::s_Serializers;

}