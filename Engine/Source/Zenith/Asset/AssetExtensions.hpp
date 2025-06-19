#pragma once

#include <unordered_map>

#include "AssetTypes.hpp"

namespace Zenith {

	inline static std::unordered_map<std::string, AssetType> s_AssetExtensionMap =
	{
		// Fonts
		{ ".ttf", AssetType::Font },
		{ ".ttc", AssetType::Font },
		{ ".otf", AssetType::Font }
	};

}