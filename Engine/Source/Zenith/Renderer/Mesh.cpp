#include "znpch.hpp" 
#include "Mesh.hpp"

#include "Zenith/Debug/Profiler.hpp"
#include "Zenith/Math/Math.hpp"
#include "Zenith/Renderer/Renderer.hpp"
#include "Zenith/Project/Project.hpp"
#include "Zenith/Asset/MeshImporter.hpp"
#include "Zenith/Asset/AssetManager.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "imgui/imgui.h"

#include <filesystem>

namespace Zenith
{

#define MESH_DEBUG_LOG 0
#if MESH_DEBUG_LOG
#define ZN_MESH_LOG(...) ZN_CORE_TRACE(__VA_ARGS__)
#define ZN_MESH_ERROR(...) ZN_CORE_ERROR(__VA_ARGS__)
#else
#define ZN_MESH_LOG(...)
#define ZN_MESH_ERROR(...)
#endif

	////////////////////////////////////////////////////////
	// MeshSource //////////////////////////////////////////
	////////////////////////////////////////////////////////
	MeshSource::MeshSource(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const glm::mat4& transform)
		: m_Vertices(vertices), m_Indices(indices)
	{
		// Generate a new asset handle
		Handle = {};

		Submesh& submesh = m_Submeshes.emplace_back();
		submesh.BaseVertex = 0;
		submesh.BaseIndex = 0;
		submesh.VertexCount = static_cast<uint32_t>(m_Vertices.size());
		submesh.IndexCount = static_cast<uint32_t>(m_Indices.size());
		submesh.Transform = transform;
		submesh.MeshName = "Default";

		CreateBuffers();

		m_BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
		m_BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
		for (const auto& vertex : m_Vertices)
		{
			m_BoundingBox.Min = glm::min(m_BoundingBox.Min, vertex.Position);
			m_BoundingBox.Max = glm::max(m_BoundingBox.Max, vertex.Position);
		}

		submesh.BoundingBox = m_BoundingBox;

		if (!ValidateIndices())
		{
			ZN_CORE_WARN_TAG("Mesh", "MeshSource created with invalid indices");
		}
	}

	MeshSource::MeshSource(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<Submesh>& submeshes)
		: m_Vertices(vertices), m_Indices(indices), m_Submeshes(submeshes)
	{
		// Generate a new asset handle
		Handle = {};

		CreateBuffers();

		m_BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
		m_BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
		for (const auto& vertex : m_Vertices)
		{
			m_BoundingBox.Min = glm::min(m_BoundingBox.Min, vertex.Position);
			m_BoundingBox.Max = glm::max(m_BoundingBox.Max, vertex.Position);
		}

		if (!ValidateIndices())
		{
			ZN_CORE_WARN_TAG("Mesh", "MeshSource created with invalid indices");
		}
	}

	void MeshSource::CreateBuffers()
	{
		if (!m_Vertices.empty())
			m_VertexBuffer = VertexBuffer::Create(m_Vertices.data(),
				static_cast<uint32_t>(m_Vertices.size() * sizeof(Vertex)));

		if (!m_Indices.empty())
			m_IndexBuffer = IndexBuffer::Create(m_Indices.data(),
				static_cast<uint32_t>(m_Indices.size() * sizeof(uint32_t)));
	}

	bool MeshSource::ValidateIndices() const
	{
		if (m_Indices.empty() || m_Vertices.empty())
			return true; // Empty is valid

		uint32_t vertexCount = static_cast<uint32_t>(m_Vertices.size());

		for (size_t i = 0; i < m_Indices.size(); ++i)
		{
			if (m_Indices[i] >= vertexCount)
			{
				ZN_MESH_ERROR("Invalid index {} at position {} in mesh '{}' (vertex count: {})",
					m_Indices[i], i, m_FilePath, vertexCount);
				return false;
			}
		}

		return true;
	}

	MeshSource::~MeshSource()
	{
	}

	static std::string LevelToSpaces(uint32_t level)
	{
		std::string result = "";
		for (uint32_t i = 0; i < level; i++)
			result += "--";
		return result;
	}

	void MeshSource::DumpVertexBuffer()
	{
		// TODO: Convert to ImGui
		ZN_MESH_LOG("------------------------------------------------------");
		ZN_MESH_LOG("Vertex Buffer Dump");
		ZN_MESH_LOG("Mesh: {0}", m_FilePath);
		for (size_t i = 0; i < m_Vertices.size(); i++)
		{
			auto& vertex = m_Vertices[i];
			ZN_MESH_LOG("Vertex: {0}", i);
			ZN_MESH_LOG("Position: {0}, {1}, {2}", vertex.Position.x, vertex.Position.y, vertex.Position.z);
			ZN_MESH_LOG("Normal: {0}, {1}, {2}", vertex.Normal.x, vertex.Normal.y, vertex.Normal.z);
			ZN_MESH_LOG("Binormal: {0}, {1}, {2}", vertex.Binormal.x, vertex.Binormal.y, vertex.Binormal.z);
			ZN_MESH_LOG("Tangent: {0}, {1}, {2}", vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z);
			ZN_MESH_LOG("TexCoord: {0}, {1}", vertex.Texcoord.x, vertex.Texcoord.y);
			ZN_MESH_LOG("--");
		}
		ZN_MESH_LOG("------------------------------------------------------");
	}

	////////////////////////////////////////////////////////
	// StaticMesh //////////////////////////////////////////
	////////////////////////////////////////////////////////

	StaticMesh::StaticMesh(AssetHandle meshSource)
		: m_MeshSource(meshSource)
	{
		// Generate a new asset handle
		Handle = {};

		// Make sure to create material table even if meshsource asset cannot be retrieved
		// (this saves having to keep checking mesh->m_Materials is not null elsewhere in the code)
		m_Materials = Ref<MaterialTable>::Create(0);

		if(auto meshSourceAsset = AssetManager::GetAsset<MeshSource>(meshSource); meshSourceAsset)
		{
			SetSubmeshes({}, meshSourceAsset);

			const std::vector<AssetHandle>& meshMaterials = meshSourceAsset->GetMaterials();
			uint32_t numMaterials = static_cast<uint32_t>(meshMaterials.size());
			for (uint32_t i = 0; i < numMaterials; i++)
				m_Materials->SetMaterial(i, meshMaterials[i]);
		}
	}

	StaticMesh::StaticMesh(AssetHandle meshSource, const std::vector<uint32_t>& submeshes)
		: m_MeshSource(meshSource)
	{
		// Generate a new asset handle
		Handle = {};

		// Make sure to create material table even if meshsource asset cannot be retrieved
		// (this saves having to keep checking mesh->m_Materials is not null elsewhere in the code)
		m_Materials = Ref<MaterialTable>::Create(0);

		if (auto meshSourceAsset = AssetManager::GetAsset<MeshSource>(meshSource); meshSourceAsset)
		{
			SetSubmeshes(submeshes, meshSourceAsset);

			const std::vector<AssetHandle>& meshMaterials = meshSourceAsset->GetMaterials();
			uint32_t numMaterials = static_cast<uint32_t>(meshMaterials.size());
			for (uint32_t i = 0; i < numMaterials; i++)
				m_Materials->SetMaterial(i, meshMaterials[i]);
		}
	}

	void StaticMesh::OnDependencyUpdated(AssetHandle)
	{
		Project::GetAssetManager()->ReloadDataAsync(Handle);
	}

	void StaticMesh::SetSubmeshes(const std::vector<uint32_t>& submeshes, Ref<MeshSource> meshSource)
	{
		if (!submeshes.empty())
		{
			m_Submeshes = submeshes;
		}
		else
		{
			const auto& submeshes = meshSource->GetSubmeshes();
			m_Submeshes.resize(submeshes.size());
			for (uint32_t i = 0; i < submeshes.size(); i++)
				m_Submeshes[i] = i;
		}
	}
}