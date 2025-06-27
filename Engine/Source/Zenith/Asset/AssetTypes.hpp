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
		Scene,
		Mesh,
		StaticMesh,
		MeshSource,
		Material,
		Texture,
		Font
	};

	namespace Utils {

		inline AssetType AssetTypeFromString(std::string_view assetType)
		{
			if (assetType == "None")                return AssetType::None;
			if (assetType == "Scene")               return AssetType::Scene;
			if (assetType == "Mesh")                return AssetType::Mesh;
			if (assetType == "StaticMesh")          return AssetType::StaticMesh;
			if (assetType == "MeshSource")          return AssetType::MeshSource;
			if (assetType == "Material")            return AssetType::Material;
			if (assetType == "Texture")             return AssetType::Texture;
			if (assetType == "Font")                return AssetType::Font;

			return AssetType::None;
		}

		inline const char* AssetTypeToString(AssetType assetType)
		{
			switch (assetType)
			{
				case AssetType::None:                return "None";
				case AssetType::Scene:               return "Scene";
				case AssetType::Mesh:                return "Mesh";
				case AssetType::StaticMesh:          return "StaticMesh";
				case AssetType::MeshSource:          return "MeshSource";
				case AssetType::Material:            return "Material";
				case AssetType::Texture:             return "Texture";
				case AssetType::Font:                return "Font";
			}

			ZN_CORE_ASSERT(false, "Unknown Asset Type");
			return "None";
		}

	}
}