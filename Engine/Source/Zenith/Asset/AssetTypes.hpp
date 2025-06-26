#pragma once

#include "Zenith/Core/Assert.hpp"

namespace Zenith {

	enum class AssetFlag : uint16_t
	{
		None = 0,
		Missing = BIT(0),
		Invalid = BIT(1)
	};

	enum class AssetType : uint16_t
	{
		None = 0,
		Font,
		Texture
	};

	namespace Utils {

		inline AssetType AssetTypeFromString(std::string_view assetType)
		{
			if (assetType == "None")                return AssetType::None;
			if (assetType == "Font")                return AssetType::Font;
			if (assetType == "Texture")             return AssetType::Texture;
			return AssetType::None;
		}

		inline const char* AssetTypeToString(AssetType assetType)
		{
			switch (assetType)
			{
				case AssetType::None:                return "None";
				case AssetType::Font:                return "Font";
				case AssetType::Texture:             return "Texture";
			}

			ZN_CORE_ASSERT(false, "Unknown Asset Type");
			return "None";
		}

	}
}