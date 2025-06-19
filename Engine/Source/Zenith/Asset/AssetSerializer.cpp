#include "znpch.hpp"
#include "AssetSerializer.hpp"

#include "AssetManager.hpp"

#include "Zenith/Renderer/Font.hpp"

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
		asset = Ref<Font>::Create(Project::GetEditorAssetManager()->GetFileSystemPathString(metadata));
		asset->Handle = metadata.Handle;

#if 0
		// TODO: we should probably handle fonts not loading correctly
		bool result = asset.As<Font>()->Loaded();
		if (!result)
			asset->SetFlag(AssetFlag::Invalid, true);
#endif

		return true;
	}

	void AssetSerializer::RegisterDependencies(const AssetMetadata& metadata) const
	{
		AssetManager::RegisterDependency(AssetHandle(), metadata.Handle);
	}

}