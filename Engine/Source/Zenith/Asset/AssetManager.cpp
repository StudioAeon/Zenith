#include "znpch.hpp"
#include "AssetManager.hpp"

#include "Zenith/Renderer/Font.hpp"

namespace Zenith {

	static std::unordered_map<AssetType, std::function<Ref<Asset>()>> s_AssetPlaceholderTable =
	{
		{ AssetType::Font, []() { return Font::GetDefaultFont(); }}
	};

	Ref<Asset> AssetManager::GetPlaceholderAsset(AssetType type)
	{
		if (s_AssetPlaceholderTable.contains(type))
			return s_AssetPlaceholderTable.at(type)();

		return nullptr;
	}
}