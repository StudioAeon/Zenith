#pragma once

#include "Zenith/Renderer/Mesh.hpp"
#include "Zenith/Renderer/MaterialAsset.hpp"
#include "Zenith/Asset/TextureImporter.hpp" // UPDATED: Now includes TextureData

#include <filesystem>
#include <functional>

namespace fastgltf { struct Asset; }

namespace Zenith {

	// Utility structs for vertex deduplication in OBJ import
	struct VertexKey {
		uint32_t p, n, t;
		bool operator==(const VertexKey& other) const {
			return p == other.p && n == other.n && t == other.t;
		}
	};

	struct VertexKeyHash {
		size_t operator()(const VertexKey& key) const {
			return ((std::hash<uint32_t>()(key.p) ^
				(std::hash<uint32_t>()(key.n) << 1)) >> 1) ^
				(std::hash<uint32_t>()(key.t) << 1);
		}
	};

	enum class MeshFormat
	{
		Unknown,
		FBX,
		GLTF,
		GLB,
		OBJ
	};

	class MeshImporter
	{
	public:
		MeshImporter(const std::filesystem::path& path);

		Ref<MeshSource> ImportToMeshSource();

	private:
		MeshFormat DetectFormat() const;

		Ref<MeshSource> ImportFBX();
		Ref<MeshSource> ImportGLTF();
		Ref<MeshSource> ImportOBJ();

		void ProcessNode(Ref<MeshSource> meshSource, void* node, uint32_t nodeIndex,
			const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);
		void CalculateBoundingBox(Ref<MeshSource> meshSource);

		void ProcessMaterials(Ref<MeshSource> meshSource, void* scene, MeshFormat format);
		AssetHandle CreateMaterialFromTexture(const std::string& texturePath, const std::string& name = "");

		Ref<MaterialAsset> CreateMaterialFromGLTF(const fastgltf::Asset& asset, size_t materialIndex);
		AssetHandle ProcessGLTFTexture(const fastgltf::Asset& asset, size_t textureIndex, const std::string& semanticName);
		AssetHandle LoadImageFromGLTF(const fastgltf::Asset& asset, size_t imageIndex, const std::string& debugName);

		void CreateMeshBuffers(Ref<MeshSource> meshSource);

		static glm::mat4 ToGLMMat4(const float* matrix);
		static glm::vec3 ToGLMVec3(const float* vec);
		static glm::quat ToGLMQuat(const float* quat);

	private:
		const std::filesystem::path m_Path;
		MeshFormat m_Format;
	};

}