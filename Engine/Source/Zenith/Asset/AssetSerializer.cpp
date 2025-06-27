#include "znpch.hpp"
#include "AssetSerializer.hpp"

#include "AssetManager.hpp"

#include "Zenith/Renderer/MaterialAsset.hpp"
#include "Zenith/Renderer/Mesh.hpp"
#include "Zenith/Renderer/Renderer.hpp"
// #include "Zenith/Renderer/Font.hpp"

#include "Zenith/Utilities/FileSystem.hpp"
#include "Zenith/Utilities/SerializationMacros.hpp"
#include "Zenith/Utilities/StringUtils.hpp"
#include "Zenith/Utilities/JSONSerializationHelpers.hpp"

#include <nlohmann/json.hpp>

namespace Zenith {

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
	// MaterialAssetSerializer
	//////////////////////////////////////////////////////////////////////////////////

	using json = nlohmann::json;

	void MaterialAssetSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<MaterialAsset> materialAsset = asset.As<MaterialAsset>();

		std::string jsonString = SerializeToJSON(materialAsset);

		std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		fout << jsonString;
	}

	bool MaterialAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		try
		{
			Ref<MaterialAsset> materialAsset;
			if (!DeserializeFromJSON(GetJSON(metadata), materialAsset, metadata.Handle))
			{
				return false;
			}
			asset = materialAsset;
			return true;
		}
		catch (const json::exception& e)
		{
			ZN_CORE_ERROR("Failed to deserialize MaterialAsset: {}", e.what());
			return false;
		}
	}

	void MaterialAssetSerializer::RegisterDependencies(const AssetMetadata& metadata) const
	{
		try
		{
			RegisterDependenciesFromJSON(GetJSON(metadata), metadata.Handle);
		}
		catch (const json::exception& e)
		{
			ZN_CORE_ERROR("Failed to register dependencies for MaterialAsset: {}", e.what());
		}
	}

	std::string MaterialAssetSerializer::SerializeToJSON(Ref<MaterialAsset> materialAsset) const
	{
		json j;
		json materialJson;

		// TODO: this should have shader UUID when that's a thing
		// right now only supports PBR or Transparent shaders
		Ref<Shader> transparentShader = Renderer::GetShaderLibrary()->Get("PBR_TransparentMesh");
		bool transparent = materialAsset->GetMaterial()->GetShader() == transparentShader;

		materialJson["Transparent"] = transparent;
		materialJson["AlbedoColor"] = {
			materialAsset->GetAlbedoColor().x,
			materialAsset->GetAlbedoColor().y,
			materialAsset->GetAlbedoColor().z
		};
		materialJson["Emission"] = materialAsset->GetEmission();

		if (!transparent)
		{
			materialJson["UseNormalMap"] = materialAsset->IsUsingNormalMap();
			materialJson["Metalness"] = materialAsset->GetMetalness();
			materialJson["Roughness"] = materialAsset->GetRoughness();
		}
		else
		{
			materialJson["Transparency"] = materialAsset->GetTransparency();
		}

		// Texture maps
		{
			Ref<Texture2D> albedoMap = materialAsset->GetAlbedoMap();
			bool hasAlbedoMap = albedoMap ? !albedoMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
			AssetHandle albedoMapHandle = hasAlbedoMap ? albedoMap->Handle : UUID(0);
			materialJson["AlbedoMap"] = static_cast<uint64_t>(albedoMapHandle);
		}

		if (!transparent)
		{
			{
				Ref<Texture2D> normalMap = materialAsset->GetNormalMap();
				bool hasNormalMap = normalMap ? !normalMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
				AssetHandle normalMapHandle = hasNormalMap ? normalMap->Handle : UUID(0);
				materialJson["NormalMap"] = static_cast<uint64_t>(normalMapHandle);
			}
			{
				Ref<Texture2D> metalnessMap = materialAsset->GetMetalnessMap();
				bool hasMetalnessMap = metalnessMap ? !metalnessMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
				AssetHandle metalnessMapHandle = hasMetalnessMap ? metalnessMap->Handle : UUID(0);
				materialJson["MetalnessMap"] = static_cast<uint64_t>(metalnessMapHandle);
			}
			{
				Ref<Texture2D> roughnessMap = materialAsset->GetRoughnessMap();
				bool hasRoughnessMap = roughnessMap ? !roughnessMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
				AssetHandle roughnessMapHandle = hasRoughnessMap ? roughnessMap->Handle : UUID(0);
				materialJson["RoughnessMap"] = static_cast<uint64_t>(roughnessMapHandle);
			}
		}

		materialJson["MaterialFlags"] = materialAsset->GetMaterial()->GetFlags();

		j["Material"] = materialJson;

		return j.dump(4);
	}

	std::string MaterialAssetSerializer::GetJSON(const AssetMetadata& metadata) const
	{
		std::ifstream stream(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		if (!stream.is_open())
			return std::string();

		std::stringstream strStream;
		strStream << stream.rdbuf();
		return strStream.str();
	}

	void MaterialAssetSerializer::RegisterDependenciesFromJSON(const std::string& jsonString, AssetHandle handle) const
	{
		AssetManager::DeregisterDependencies(handle);

		json root = json::parse(jsonString);
		const auto& materialNode = root["Material"];

		AssetHandle albedoMap = AssetHandle(materialNode.value("AlbedoMap", static_cast<uint64_t>(0)));
		AssetHandle normalMap = AssetHandle(materialNode.value("NormalMap", static_cast<uint64_t>(0)));
		AssetHandle metalnessMap = AssetHandle(materialNode.value("MetalnessMap", static_cast<uint64_t>(0)));
		AssetHandle roughnessMap = AssetHandle(materialNode.value("RoughnessMap", static_cast<uint64_t>(0)));

		// note: we should always register something, even 0.
		AssetManager::RegisterDependency(albedoMap, handle);
		AssetManager::RegisterDependency(normalMap, handle);
		AssetManager::RegisterDependency(metalnessMap, handle);
		AssetManager::RegisterDependency(roughnessMap, handle);
	}

	bool MaterialAssetSerializer::DeserializeFromJSON(const std::string& jsonString, Ref<MaterialAsset>& targetMaterialAsset, AssetHandle handle) const
	{
		RegisterDependenciesFromJSON(jsonString, handle);

		json root = json::parse(jsonString);
		const auto& materialNode = root["Material"];

		bool transparent = materialNode.value("Transparent", false);

		targetMaterialAsset = Ref<MaterialAsset>::Create(transparent);
		targetMaterialAsset->Handle = handle;

		// Deserialize albedo color
		if (materialNode.contains("AlbedoColor") && materialNode["AlbedoColor"].is_array())
		{
			auto albedoArray = materialNode["AlbedoColor"];
			if (albedoArray.size() >= 3)
			{
				targetMaterialAsset->GetAlbedoColor() = glm::vec3(
					albedoArray[0].get<float>(),
					albedoArray[1].get<float>(),
					albedoArray[2].get<float>()
				);
			}
		}
		else
		{
			targetMaterialAsset->GetAlbedoColor() = glm::vec3(0.8f);
		}

		targetMaterialAsset->GetEmission() = materialNode.value("Emission", 0.0f);

		if (!transparent)
		{
			targetMaterialAsset->SetUseNormalMap(materialNode.value("UseNormalMap", false));
			targetMaterialAsset->GetMetalness() = materialNode.value("Metalness", 0.0f);
			targetMaterialAsset->GetRoughness() = materialNode.value("Roughness", 0.5f);
		}
		else
		{
			targetMaterialAsset->GetTransparency() = materialNode.value("Transparency", 1.0f);
		}

		// Load texture maps
		if (materialNode.contains("AlbedoMap") && materialNode["AlbedoMap"].get<uint64_t>() != 0)
		{
			AssetHandle albedoMap = AssetHandle(materialNode["AlbedoMap"].get<uint64_t>());
			if (AssetManager::IsAssetHandleValid(albedoMap))
				targetMaterialAsset->SetAlbedoMap(albedoMap);
		}

		if (!transparent)
		{
			if (materialNode.contains("NormalMap") && materialNode["NormalMap"].get<uint64_t>() != 0)
			{
				AssetHandle normalMap = AssetHandle(materialNode["NormalMap"].get<uint64_t>());
				if (AssetManager::IsAssetHandleValid(normalMap))
					targetMaterialAsset->SetNormalMap(normalMap);
			}

			if (materialNode.contains("MetalnessMap") && materialNode["MetalnessMap"].get<uint64_t>() != 0)
			{
				AssetHandle metalnessMap = AssetHandle(materialNode["MetalnessMap"].get<uint64_t>());
				if (AssetManager::IsAssetHandleValid(metalnessMap))
					targetMaterialAsset->SetMetalnessMap(metalnessMap);
			}

			if (materialNode.contains("RoughnessMap") && materialNode["RoughnessMap"].get<uint64_t>() != 0)
			{
				AssetHandle roughnessMap = AssetHandle(materialNode["RoughnessMap"].get<uint64_t>());
				if (AssetManager::IsAssetHandleValid(roughnessMap))
					targetMaterialAsset->SetRoughnessMap(roughnessMap);
			}
		}

		if (materialNode.contains("MaterialFlags"))
			targetMaterialAsset->GetMaterial()->SetFlags(materialNode["MaterialFlags"].get<uint32_t>());

		return true;
	}

	void AssetSerializer::RegisterDependencies(const AssetMetadata& metadata) const
	{
		AssetManager::RegisterDependency(AssetHandle(), metadata.Handle);
	}

}