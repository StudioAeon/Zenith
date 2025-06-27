#pragma once

#include "Zenith/Renderer/Mesh.hpp"

#include <filesystem>

namespace Zenith {

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

		static glm::mat4 ToGLMMat4(const float* matrix);
		static glm::vec3 ToGLMVec3(const float* vec);
		static glm::quat ToGLMQuat(const float* quat);

	private:
		const std::filesystem::path m_Path;
		MeshFormat m_Format;
	};

}