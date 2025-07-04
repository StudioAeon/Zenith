#pragma once

#include "Zenith/Asset/Asset.hpp"

#include "Zenith/Math/AABB.hpp"

#include "Zenith/Renderer/IndexBuffer.hpp"
#include "Zenith/Renderer/MaterialAsset.hpp"
#include "Zenith/Renderer/UniformBuffer.hpp"
#include "Zenith/Renderer/VertexBuffer.hpp"

#include <vector>
#include <glm/glm.hpp>

namespace Zenith {

	struct Vertex
	{
		glm::vec3 Position = {0.0f, 0.0f, 0.0f};
		glm::vec3 Normal = {0.0f, 0.0f, 1.0f};
		glm::vec3 Tangent = {1.0f, 0.0f, 0.0f};
		glm::vec3 Binormal = {0.0f, 1.0f, 0.0f};
		glm::vec2 Texcoord = {0.0f, 0.0f};
	};

	static const int NumAttributes = 5;

	struct Triangle
	{
		Vertex V0, V1, V2;

		Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2)
			: V0(v0), V1(v1), V2(v2) {}
	};

	class Submesh
	{
	public:
		uint32_t BaseVertex = 0;
		uint32_t BaseIndex = 0;
		uint32_t MaterialIndex = 0;
		uint32_t IndexCount = 0;
		uint32_t VertexCount = 0;

		glm::mat4 Transform{ 1.0f }; // World transform
		glm::mat4 LocalTransform{ 1.0f };
		AABB BoundingBox;

		std::string NodeName, MeshName;

		static void Serialize(StreamWriter* serializer, const Submesh& instance)
		{
			serializer->WriteRaw(instance.BaseVertex);
			serializer->WriteRaw(instance.BaseIndex);
			serializer->WriteRaw(instance.MaterialIndex);
			serializer->WriteRaw(instance.IndexCount);
			serializer->WriteRaw(instance.VertexCount);
			serializer->WriteRaw(instance.Transform);
			serializer->WriteRaw(instance.LocalTransform);
			serializer->WriteRaw(instance.BoundingBox);
			serializer->WriteString(instance.NodeName);
			serializer->WriteString(instance.MeshName);
		}

		static void Deserialize(StreamReader* deserializer, Submesh& instance)
		{
			deserializer->ReadRaw(instance.BaseVertex);
			deserializer->ReadRaw(instance.BaseIndex);
			deserializer->ReadRaw(instance.MaterialIndex);
			deserializer->ReadRaw(instance.IndexCount);
			deserializer->ReadRaw(instance.VertexCount);
			deserializer->ReadRaw(instance.Transform);
			deserializer->ReadRaw(instance.LocalTransform);
			deserializer->ReadRaw(instance.BoundingBox);
			deserializer->ReadString(instance.NodeName);
			deserializer->ReadString(instance.MeshName);
		}
	};

	struct MeshNode
	{
		uint32_t Parent = 0xffffffff;
		std::vector<uint32_t> Children;
		std::vector<uint32_t> Submeshes;

		std::string Name;
		glm::mat4 LocalTransform;

		inline bool IsRoot() const { return Parent == 0xffffffff; }

		static void Serialize(StreamWriter* serializer, const MeshNode& instance)
		{
			serializer->WriteRaw(instance.Parent);
			serializer->WriteArray(instance.Children);
			serializer->WriteArray(instance.Submeshes);
			serializer->WriteString(instance.Name);
			serializer->WriteRaw(instance.LocalTransform);
		}

		static void Deserialize(StreamReader* deserializer, MeshNode& instance)
		{
			deserializer->ReadRaw(instance.Parent);
			deserializer->ReadArray(instance.Children);
			deserializer->ReadArray(instance.Submeshes);
			deserializer->ReadString(instance.Name);
			deserializer->ReadRaw(instance.LocalTransform);
		}
	};

	//
	// MeshSource is a representation of an actual asset file on disk
	// Meshes are created from MeshSource
	//
	class MeshSource : public Asset
	{
	public:
		MeshSource() = default;
		MeshSource(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const glm::mat4& transform = glm::mat4(1.0f));
		MeshSource(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<Submesh>& submeshes);
		virtual ~MeshSource();

		const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
		const std::vector<uint32_t>& GetIndices() const { return m_Indices; }
		const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }

		std::vector<Vertex>& GetVertices() { return m_Vertices; }
		std::vector<uint32_t>& GetIndices() { return m_Indices; }
		std::vector<Submesh>& GetSubmeshes() { return m_Submeshes; }

		Ref<VertexBuffer> GetVertexBuffer() const { return m_VertexBuffer; }
		Ref<IndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }

		const std::vector<MeshNode>& GetNodes() const { return m_Nodes; }
		const std::vector<uint32_t>& GetRootNodes() const { return m_RootNodes; }

		const std::vector<AssetHandle>& GetMaterials() const { return m_Materials; }
		const AABB& GetBoundingBox() const { return m_BoundingBox; }

		const std::string& GetFilePath() const { return m_FilePath; }

		static AssetType GetStaticType() { return AssetType::MeshSource; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		void DumpVertexBuffer();

	private:
		std::vector<Vertex> m_Vertices;
		std::vector<uint32_t> m_Indices;
		std::vector<Submesh> m_Submeshes;

		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;

		std::vector<MeshNode> m_Nodes;
		std::vector<uint32_t> m_RootNodes;

		std::vector<AssetHandle> m_Materials;
		AABB m_BoundingBox;

		std::string m_FilePath;

		void CreateBuffers();

		bool ValidateIndices() const;

		friend class MeshImporter;
	};

	// Static Mesh - no skeletal animation, flattened hierarchy
	class StaticMesh : public Asset
	{
	public:
		explicit StaticMesh(AssetHandle meshSource);
		StaticMesh(AssetHandle meshSource, const std::vector<uint32_t>& submeshes);
		virtual ~StaticMesh() = default;

		virtual void OnDependencyUpdated(AssetHandle handle) override;

		const std::vector<uint32_t>& GetSubmeshes() const { return m_Submeshes; }

		void SetSubmeshes(const std::vector<uint32_t>& submeshes, Ref<MeshSource> meshSourceAsset);

		AssetHandle GetMeshSource() const { return m_MeshSource; }
		void SetMeshAsset(AssetHandle meshSource) { m_MeshSource = meshSource; }

		Ref<MaterialTable> GetMaterials() const { return m_Materials; }

		static AssetType GetStaticType() { return AssetType::StaticMesh; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	private:
		AssetHandle m_MeshSource;
		std::vector<uint32_t> m_Submeshes; // TODO: physics/render masks

		Ref<MaterialTable> m_Materials;

		friend class Renderer;
		friend class VulkanRenderer;
	};

}