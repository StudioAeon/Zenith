#pragma once

#include "Asset.hpp"

#include <filesystem>

namespace Zenith {

	enum class AssetStatus
	{
		None = 0, Ready = 1, Invalid = 2, Loading = 3
	};

	struct AssetMetadata
	{
		AssetHandle Handle = AssetHandle::null();
		AssetType Type;
		std::filesystem::path FilePath;

		AssetStatus Status = AssetStatus::None;

		uint64_t FileLastWriteTime = 0; // TODO: this is the last write time of the file WE LOADED
		bool IsDataLoaded = false;

		bool IsValid() const { return Handle != 0; }
	};

	struct EditorAssetLoadResponse
	{
		AssetMetadata Metadata;
		Ref<Asset> Asset;
	};

}