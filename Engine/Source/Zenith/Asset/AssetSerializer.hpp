#pragma once

#include "AssetMetadata.hpp"

#include "Zenith/Serialization/FileStream.hpp"

namespace Zenith {

	class MaterialAsset;

	struct AssetSerializationInfo
	{
		uint64_t Offset = 0;
		uint64_t Size = 0;
	};

	class AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const = 0;
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const = 0;
		virtual void RegisterDependencies(const AssetMetadata& metadata) const;
	};

	class TextureSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override{}
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
	};

	class FontSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override {}
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
	};

	class MaterialAssetSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
		virtual void RegisterDependencies(const AssetMetadata& metadata) const override;
	private:
		std::string SerializeToJSON(Ref<MaterialAsset> materialAsset) const;
		std::string GetJSON(const AssetMetadata& metadata) const;
		void RegisterDependenciesFromJSON(const std::string& yamlString, AssetHandle handle) const;
		bool DeserializeFromJSON(const std::string& yamlString, Ref<MaterialAsset>& targetMaterialAsset, AssetHandle handle) const;
	};

}