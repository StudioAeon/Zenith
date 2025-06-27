#pragma once

#include <unordered_map>

#include "AssetTypes.hpp"

namespace Zenith {

	inline static std::unordered_map<std::string, AssetType> s_AssetExtensionMap =
	{
		// Zenith types
		{ ".zscene", AssetType::Scene },
		{ ".zmesh", AssetType::Mesh },
		{ ".zsmesh", AssetType::StaticMesh },
		{ ".zmaterial", AssetType::Material },

		// mesh/animation source
		{ ".fbx", AssetType::MeshSource },
		{ ".gltf", AssetType::MeshSource },
		{ ".glb", AssetType::MeshSource },
		{ ".obj", AssetType::MeshSource },

		// Textures
		{ ".png", AssetType::Texture },
		{ ".jpg", AssetType::Texture },
		{ ".jpeg", AssetType::Texture },

		// Fonts
		{ ".ttf", AssetType::Font },
		{ ".ttc", AssetType::Font },
		{ ".otf", AssetType::Font }

	};

}