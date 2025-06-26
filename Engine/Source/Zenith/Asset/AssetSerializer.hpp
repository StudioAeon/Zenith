#pragma once

#include "AssetMetadata.hpp"

#include "Zenith/Serialization/FileStream.hpp"

namespace Zenith {

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

	class FontSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override {}
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
	};

	class TextureSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override{}
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
	};

}