#include "znpch.hpp"
#include "MeshSerializer.hpp"

#include <nlohmann/json.hpp>

#include "Zenith/Asset/AssetManager.hpp"
#include "Zenith/Project/Project.hpp"

#include "MeshImporter.hpp"

#include "Zenith/Debug/Profiler.hpp"

using json = nlohmann::json;

namespace Zenith {

	static std::string GetJSONFromFile(const AssetMetadata& metadata)
	{
		std::ifstream stream(Project::GetActiveAssetDirectory() / metadata.FilePath);
		ZN_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();
		return strStream.str();
	}

	//////////////////////////////////////////////////////////////////////////////////
	// StaticMeshSerializer
	//////////////////////////////////////////////////////////////////////////////////

	std::string SerializeToJSON(Ref<StaticMesh> staticMesh)
	{
		json j;
		j["Mesh"] = json{
			{"MeshSource", static_cast<uint64_t>(staticMesh->GetMeshSource())},
			{"SubmeshIndices", staticMesh->GetSubmeshes()}
		};

		return j.dump(4);
	}

	bool DeserializeFromJSON(const json& data, Ref<StaticMesh>& targetStaticMesh)
	{
		if (!data.contains("Mesh"))
			return false;

		const auto& meshNode = data["Mesh"];

		AssetHandle meshSource = AssetHandle{0};

		// Don't return false here if the MeshSource is missing.
		// We have still loaded the asset, its just invalid.
		// This allows:
		//   - the thumbnail generator to generate a thumbnail for this mesh (it will be the "invalid" thumbnail,
		//     giving the user visual feedback that something is wrong)
		//   - asset pack builder to report missing mesh source (as opposed to just silently skipping this mesh)

		std::vector<uint32_t> submeshIndices;
		if (meshNode.contains("SubmeshIndices"))
		{
			submeshIndices = meshNode["SubmeshIndices"].get<std::vector<uint32_t>>();
		}

		targetStaticMesh = Ref<StaticMesh>::Create(meshSource, submeshIndices);
		return true;
	}

	void RegisterStaticMeshDependenciesFromJSON(const json& data, AssetHandle handle)
	{
		Project::GetEditorAssetManager()->DeregisterDependencies(handle);
		AssetHandle meshSourceHandle = AssetHandle{0};

		if (data.contains("Mesh"))
		{
			const auto& meshNode = data["Mesh"];
			if (meshNode.contains("MeshSource"))
				meshSourceHandle = AssetHandle{meshNode["MeshSource"].get<uint64_t>()};
		}

		// must always register something, even if it's 0
		Project::GetEditorAssetManager()->RegisterDependency(meshSourceHandle, handle);
	}

	bool MeshSourceSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		ZN_PROFILE_FUNC("MeshSourceSerializer::TryLoadData");

		MeshImporter importer(Project::GetEditorAssetManager()->GetFileSystemPathString(metadata));
		Ref<MeshSource> meshSource = importer.ImportToMeshSource();
		if (!meshSource)
			return false;

		asset = meshSource;
		asset->Handle = metadata.Handle;
		return true;
	}

	void StaticMeshSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<StaticMesh> staticMesh = asset.As<StaticMesh>();

		std::string jsonString = SerializeToJSON(staticMesh);

		auto serializePath = Project::GetActive()->GetAssetDirectory() / metadata.FilePath;
		std::ofstream fout(serializePath);
		ZN_CORE_ASSERT(fout.good());
		if (fout.good())
			fout << jsonString;
	}

	bool StaticMeshSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		Ref<StaticMesh> staticMesh;
		std::string jsonStr = GetJSONFromFile(metadata);

		try
		{
			json data = json::parse(jsonStr);
			bool success = DeserializeFromJSON(data, staticMesh);
			if (!success)
				return false;

			staticMesh->Handle = metadata.Handle;
			RegisterStaticMeshDependenciesFromJSON(data, staticMesh->Handle);
			asset = staticMesh;
			return true;
		}
		catch (const json::exception& e)
		{
			ZN_CORE_ERROR_TAG("Serialization", "Failed to parse JSON for StaticMesh {0}: {1}", metadata.FilePath.string(), e.what());
			return false;
		}
	}

	void StaticMeshSerializer::RegisterDependencies(const AssetMetadata& metadata) const
	{
		try
		{
			json data = json::parse(GetJSONFromFile(metadata));
			RegisterStaticMeshDependenciesFromJSON(data, metadata.Handle);
		}
		catch (const json::exception& e)
		{
			ZN_CORE_ERROR_TAG("Serialization", "Failed to parse JSON for dependency registration {0}: {1}", metadata.FilePath.string(), e.what());
		}
	}

}