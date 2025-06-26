#include "znpch.hpp"
#include "AssetSerializer.hpp"

#include "AssetManager.hpp"

#include "Zenith/Renderer/Renderer.hpp"
// #include "Zenith/Renderer/Font.hpp"

#include "Zenith/Utilities/FileSystem.hpp"
#include "Zenith/Utilities/SerializationMacros.hpp"
#include "Zenith/Utilities/StringUtils.hpp"
#include "Zenith/Utilities/JSONSerializationHelpers.hpp"

#include <nlohmann/json.hpp>

namespace Zenith {

	//////////////////////////////////////////////////////////////////////////////////
	// FontSerializer
	//////////////////////////////////////////////////////////////////////////////////

	bool FontSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		// asset = Ref<Font>::Create(Project::GetEditorAssetManager()->GetFileSystemPathString(metadata));
		asset->Handle = metadata.Handle;

#if 0
		// TODO: we should probably handle fonts not loading correctly
		bool result = asset.As<Font>()->Loaded();
		if (!result)
			asset->SetFlag(AssetFlag::Invalid, true);
#endif

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// TextureSerializer
	//////////////////////////////////////////////////////////////////////////////////

	bool TextureSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Texture2D::Create(TextureSpecification(), Project::GetEditorAssetManager()->GetFileSystemPathString(metadata));
		asset->Handle = metadata.Handle;

		bool result = asset.As<Texture2D>()->Loaded();
		if (!result)
			asset->SetFlag(AssetFlag::Invalid, true);

		return result;
	}

	void AssetSerializer::RegisterDependencies(const AssetMetadata& metadata) const
	{
		AssetManager::RegisterDependency(AssetHandle(), metadata.Handle);
	}

}