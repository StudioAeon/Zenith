#include "znpch.hpp"
#include "MeshImporter.hpp"

#include "Zenith/Asset/AssetManager.hpp"
#include "Zenith/Renderer/Renderer.hpp"
#include "Zenith/Renderer/Texture.hpp"
#include "Zenith/Math/Math.hpp"

#include <ufbx.h>
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fast_obj.h>

#include <glm/detail/type_quat.hpp>

#include "glm/gtc/quaternion.hpp"

namespace Zenith {

#define MESH_DEBUG_LOG 1

#if MESH_DEBUG_LOG
#define ZN_MESH_LOG(...) ZN_CORE_TRACE(__VA_ARGS__)
#define ZN_MESH_ERROR(...) ZN_CORE_ERROR(__VA_ARGS__)
#else
#define ZN_MESH_LOG(...)
#define ZN_MESH_ERROR(...)
#endif

	MeshImporter::MeshImporter(const std::filesystem::path& path)
		: m_Path(path), m_Format(DetectFormat())
	{
	}

	MeshFormat MeshImporter::DetectFormat() const
	{
		std::string extension = m_Path.extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

		MeshFormat format = MeshFormat::Unknown;
		if (extension == ".fbx") format = MeshFormat::FBX;
		else if (extension == ".gltf") format = MeshFormat::GLTF;
		else if (extension == ".glb") format = MeshFormat::GLB;
		else if (extension == ".obj") format = MeshFormat::OBJ;

		return format;
	}

	static bool ValidateIndices(const std::vector<uint32_t>& indices, uint32_t vertexCount, const std::string& meshName)
	{
		for (size_t i = 0; i < indices.size(); ++i)
		{
			if (indices[i] >= vertexCount)
			{
				ZN_MESH_ERROR("Invalid index {} at position {} in mesh '{}' (vertex count: {})",
					indices[i], i, meshName, vertexCount);
				return false;
			}
		}
		return true;
	}

	Ref<MeshSource> MeshImporter::ImportToMeshSource()
	{
		Ref<MeshSource> result = nullptr;

		switch (m_Format)
		{
			case MeshFormat::FBX:
				result = ImportFBX();
				break;
			case MeshFormat::GLTF:
				result = ImportGLTF();
				break;
			case MeshFormat::GLB:
				result = ImportGLTF();
				break;
			case MeshFormat::OBJ:
				result = ImportOBJ();
				break;
			default:
				ZN_CORE_ERROR_TAG("Mesh", "Unsupported mesh format: {0}", m_Path.string());
				return nullptr;
		}

		if (result)
		{
			DebugMaterialLoading(result);
		}

		return result;
	}

	Ref<MeshSource> MeshImporter::ImportFBX()
	{
		ufbx_load_opts opts = {};
		opts.target_axes = ufbx_axes_right_handed_y_up;
		opts.target_unit_meters = 1.0f;
		opts.generate_missing_normals = true;

		ufbx_error error;
		ufbx_scene* scene = ufbx_load_file(m_Path.string().c_str(), &opts, &error);

		if (!scene)
		{
			ZN_CORE_ERROR_TAG("Mesh", "Failed to load FBX file: {0} - {1}", m_Path.string(), error.description.data);
			return nullptr;
		}

		Ref<MeshSource> meshSource = Ref<MeshSource>::Create();
		meshSource->m_FilePath = m_Path.string();

		meshSource->m_BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
		meshSource->m_BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;

		for (size_t i = 0; i < scene->meshes.count; i++)
		{
			ufbx_mesh* mesh = scene->meshes.data[i];

			Submesh& submesh = meshSource->m_Submeshes.emplace_back();
			submesh.BaseVertex = vertexCount;
			submesh.BaseIndex = indexCount;
			submesh.MaterialIndex = 0;
			submesh.MeshName = std::string(mesh->name.data, mesh->name.length);

			uint32_t triangleCount = 0;
			for (size_t fi = 0; fi < mesh->faces.count; fi++)
			{
				ufbx_face face = mesh->faces.data[fi];
				triangleCount += static_cast<uint32_t>(face.num_indices - 2);
			}

			submesh.VertexCount = static_cast<uint32_t>(mesh->num_vertices);
			submesh.IndexCount = triangleCount * 3;

			for (size_t vi = 0; vi < mesh->num_vertices; vi++)
			{
				Vertex vertex;

				ufbx_vec3 pos = ufbx_get_vertex_vec3(&mesh->vertex_position, vi);
				vertex.Position = { pos.x, pos.y, pos.z };

				if (mesh->vertex_normal.exists)
				{
					ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, vi);
					vertex.Normal = glm::normalize(glm::vec3{ normal.x, normal.y, normal.z });
				}

				if (mesh->vertex_uv.exists)
				{
					ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, vi);
					vertex.Texcoord = { uv.x, 1.0f - uv.y };
				}

				if (mesh->vertex_tangent.exists)
				{
					ufbx_vec3 tangent = ufbx_get_vertex_vec3(&mesh->vertex_tangent, vi);
					vertex.Tangent = { tangent.x, tangent.y, tangent.z };
				}

				if (mesh->vertex_bitangent.exists)
				{
					ufbx_vec3 bitangent = ufbx_get_vertex_vec3(&mesh->vertex_bitangent, vi);
					vertex.Binormal = { bitangent.x, bitangent.y, bitangent.z };
				}

				meshSource->m_Vertices.push_back(vertex);

				meshSource->m_BoundingBox.Min = glm::min(meshSource->m_BoundingBox.Min, vertex.Position);
				meshSource->m_BoundingBox.Max = glm::max(meshSource->m_BoundingBox.Max, vertex.Position);
			}

			std::vector<uint32_t> submeshIndices;
			submeshIndices.reserve(submesh.IndexCount);

			for (size_t fi = 0; fi < mesh->faces.count; fi++)
			{
				ufbx_face face = mesh->faces.data[fi];

				for (uint32_t tri = 0; tri < face.num_indices - 2; tri++)
				{
					uint32_t i0 = static_cast<uint32_t>(mesh->vertex_indices.data[face.index_begin + 0]);
					uint32_t i1 = static_cast<uint32_t>(mesh->vertex_indices.data[face.index_begin + tri + 1]);
					uint32_t i2 = static_cast<uint32_t>(mesh->vertex_indices.data[face.index_begin + tri + 2]);

					submeshIndices.push_back(i0);
					submeshIndices.push_back(i1);
					submeshIndices.push_back(i2);
				}
			}

			if (!ValidateIndices(submeshIndices, static_cast<uint32_t>(meshSource->m_Vertices.size()), submesh.MeshName))
			{
				ZN_MESH_ERROR("Skipping submesh '{}' due to invalid indices", submesh.MeshName);
				meshSource->m_Submeshes.pop_back();
				continue;
			}

			meshSource->m_Indices.insert(meshSource->m_Indices.end(), submeshIndices.begin(), submeshIndices.end());

			vertexCount += submesh.VertexCount;
			indexCount += submesh.IndexCount;

			submesh.BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
			submesh.BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

			for (uint32_t vi = submesh.BaseVertex; vi < submesh.BaseVertex + submesh.VertexCount; vi++)
			{
				const auto& pos = meshSource->m_Vertices[vi].Position;
				submesh.BoundingBox.Min = glm::min(submesh.BoundingBox.Min, pos);
				submesh.BoundingBox.Max = glm::max(submesh.BoundingBox.Max, pos);
			}

			ZN_MESH_LOG("FBX Submesh '{}': {} vertices, {} indices", submesh.MeshName, submesh.VertexCount, submesh.IndexCount);
		}

		ProcessMaterials(meshSource, scene, MeshFormat::FBX);

		if (!meshSource->m_Vertices.empty())
			meshSource->m_VertexBuffer = VertexBuffer::Create(meshSource->m_Vertices.data(),
				static_cast<uint32_t>(meshSource->m_Vertices.size() * sizeof(Vertex)));

		if (!meshSource->m_Indices.empty())
			meshSource->m_IndexBuffer = IndexBuffer::Create(meshSource->m_Indices.data(),
				static_cast<uint32_t>(meshSource->m_Indices.size() * sizeof(uint32_t)));

		ZN_MESH_LOG("FBX Import complete: {} vertices, {} indices, {} submeshes",
			meshSource->m_Vertices.size(), meshSource->m_Indices.size(), meshSource->m_Submeshes.size());

		const auto& bb = meshSource->m_BoundingBox;
		ZN_MESH_LOG("Mesh '{}' Bounds:\n  Min: ({}, {}, {})\n  Max: ({}, {}, {})\n  Size: ({}, {}, {})",
			meshSource->m_FilePath,
			bb.Min.x, bb.Min.y, bb.Min.z,
			bb.Max.x, bb.Max.y, bb.Max.z,
			bb.Max.x - bb.Min.x, bb.Max.y - bb.Min.y, bb.Max.z - bb.Min.z
		);

		ufbx_free_scene(scene);
		return meshSource;
	}

	Ref<MeshSource> MeshImporter::ImportGLTF()
	{
		fastgltf::Parser parser;

		auto data = fastgltf::GltfDataBuffer::FromPath(m_Path);
		if (data.error() != fastgltf::Error::None)
		{
			ZN_CORE_ERROR_TAG("Mesh", "Failed to load glTF file: {0}", m_Path.string());
			return nullptr;
		}

		fastgltf::Options options =
			fastgltf::Options::LoadExternalBuffers |
			fastgltf::Options::LoadExternalImages;

		auto assetResult = parser.loadGltf(data.get(), m_Path.parent_path(), options);
		if (assetResult.error() != fastgltf::Error::None)
		{
			ZN_CORE_ERROR_TAG("Mesh", "Failed to parse glTF file: {0} - Error: {1}",
				m_Path.string(), static_cast<int>(assetResult.error()));
			return nullptr;
		}

		fastgltf::Asset& asset = assetResult.get();

		Ref<MeshSource> meshSource = Ref<MeshSource>::Create();
		meshSource->m_FilePath = m_Path.string();

		meshSource->m_BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
		meshSource->m_BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;

		std::vector<std::vector<uint32_t>> meshToSubmeshes(asset.meshes.size());
		uint32_t currentSubmeshIndex = 0;

		for (size_t meshIndex = 0; meshIndex < asset.meshes.size(); meshIndex++)
		{
			const auto& mesh = asset.meshes[meshIndex];

			for (size_t primIndex = 0; primIndex < mesh.primitives.size(); primIndex++)
			{
				const auto& primitive = mesh.primitives[primIndex];

				if (primitive.type != fastgltf::PrimitiveType::Triangles)
					continue;

				meshToSubmeshes[meshIndex].push_back(currentSubmeshIndex);

				Submesh& submesh = meshSource->m_Submeshes.emplace_back();
				submesh.BaseVertex = vertexCount;
				submesh.BaseIndex = indexCount;
				submesh.MaterialIndex = primitive.materialIndex ?
					static_cast<uint32_t>(*primitive.materialIndex) : 0;
				submesh.MeshName = std::string(mesh.name);

				std::vector<glm::vec3> positions;
				std::vector<glm::vec3> normals;
				std::vector<glm::vec2> texCoords;
				std::vector<glm::vec4> tangents;

				if (auto it = primitive.findAttribute("POSITION"); it != primitive.attributes.end())
				{
					const auto& accessor = asset.accessors[it->accessorIndex];
					fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, accessor, [&](fastgltf::math::fvec3 pos) {
						positions.emplace_back(pos.x(), pos.y(), pos.z());
					});
				}

				if (auto it = primitive.findAttribute("NORMAL"); it != primitive.attributes.end())
				{
					const auto& accessor = asset.accessors[it->accessorIndex];
					fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, accessor, [&](fastgltf::math::fvec3 normal) {
						normals.emplace_back(normal.x(), normal.y(), normal.z());
					});
				}

				if (auto it = primitive.findAttribute("TEXCOORD_0"); it != primitive.attributes.end())
				{
					const auto& accessor = asset.accessors[it->accessorIndex];
					fastgltf::iterateAccessor<fastgltf::math::fvec2>(asset, accessor, [&](fastgltf::math::fvec2 uv) {
						texCoords.emplace_back(uv.x(), uv.y());
					});
				}

				if (auto it = primitive.findAttribute("TANGENT"); it != primitive.attributes.end())
				{
					const auto& accessor = asset.accessors[it->accessorIndex];
					fastgltf::iterateAccessor<fastgltf::math::fvec4>(asset, accessor, [&](fastgltf::math::fvec4 tangent) {
						tangents.emplace_back(tangent.x(), tangent.y(), tangent.z(), tangent.w());
					});
				}

				size_t numVertices = positions.size();
				submesh.VertexCount = static_cast<uint32_t>(numVertices);

				std::vector<Vertex> localVertices;
				localVertices.reserve(numVertices);

				for (size_t i = 0; i < numVertices; i++)
				{
					Vertex vertex;
					vertex.Position = positions[i];

					if (i < normals.size())
						vertex.Normal = normals[i];

					if (i < texCoords.size())
						vertex.Texcoord = texCoords[i];

					if (i < tangents.size())
					{
						vertex.Tangent = glm::vec3(tangents[i]);
						if (glm::length(vertex.Normal) > 0.0f)
						{
							vertex.Binormal = glm::cross(vertex.Normal, vertex.Tangent) * tangents[i].w;
						}
					}

					localVertices.push_back(vertex);
				}

				submesh.IndexCount = 0;
				std::vector<uint32_t> submeshIndices;

				if (primitive.indicesAccessor)
				{
					const auto& indexAccessor = asset.accessors[*primitive.indicesAccessor];
					submesh.IndexCount = static_cast<uint32_t>(indexAccessor.count);

					submeshIndices.reserve(submesh.IndexCount);

					switch (indexAccessor.componentType)
					{
						case fastgltf::ComponentType::UnsignedShort:
						{
							fastgltf::iterateAccessor<std::uint16_t>(asset, indexAccessor, [&](std::uint16_t index) {
								submeshIndices.push_back(static_cast<uint32_t>(index));
								});
							break;
						}
						case fastgltf::ComponentType::UnsignedInt:
						{
							fastgltf::iterateAccessor<std::uint32_t>(asset, indexAccessor, [&](std::uint32_t index) {
								submeshIndices.push_back(index);
								});
							break;
						}
						default:
							ZN_MESH_ERROR("Unsupported index component type: {}", static_cast<int>(indexAccessor.componentType));
							break;
					}
					bool flipWinding = true;
					if (flipWinding) {
						for (size_t i = 0; i + 2 < submeshIndices.size(); i += 3)
							std::swap(submeshIndices[i + 1], submeshIndices[i + 2]);
					}
				}

				if (!ValidateIndices(submeshIndices, vertexCount + static_cast<uint32_t>(localVertices.size()), submesh.MeshName))
				{
					ZN_MESH_ERROR("Skipping submesh '{}' due to invalid indices", submesh.MeshName);
					meshSource->m_Submeshes.pop_back();
					continue;
				}

				meshSource->m_Vertices.insert(meshSource->m_Vertices.end(), localVertices.begin(), localVertices.end());
				meshSource->m_Indices.insert(meshSource->m_Indices.end(), submeshIndices.begin(), submeshIndices.end());

				vertexCount += static_cast<uint32_t>(localVertices.size());
				indexCount += submesh.IndexCount;

				submesh.BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
				submesh.BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

				for (uint32_t vi = submesh.BaseVertex; vi < submesh.BaseVertex + submesh.VertexCount; vi++)
				{
					const auto& pos = meshSource->m_Vertices[vi].Position;
					submesh.BoundingBox.Min = glm::min(submesh.BoundingBox.Min, pos);
					submesh.BoundingBox.Max = glm::max(submesh.BoundingBox.Max, pos);
				}

				meshSource->m_BoundingBox.Min = glm::min(meshSource->m_BoundingBox.Min, submesh.BoundingBox.Min);
				meshSource->m_BoundingBox.Max = glm::max(meshSource->m_BoundingBox.Max, submesh.BoundingBox.Max);

				currentSubmeshIndex++;
			}
		}

		ZN_MESH_LOG("Processing {} GLTF nodes", asset.nodes.size());

		if (!asset.nodes.empty())
		{
			meshSource->m_Nodes.resize(asset.nodes.size());

			for (size_t nodeIndex = 0; nodeIndex < asset.nodes.size(); nodeIndex++)
			{
				const auto& gltfNode = asset.nodes[nodeIndex];
				MeshNode& node = meshSource->m_Nodes[nodeIndex];

				node.Name = gltfNode.name;

				std::visit(fastgltf::visitor {
					[&](const fastgltf::TRS& trs) {
						glm::vec3 translation(trs.translation[0], trs.translation[1], trs.translation[2]);
						glm::quat rotation(trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]); // w, x, y, z
						glm::vec3 scale(trs.scale[0], trs.scale[1], trs.scale[2]);

						glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
						glm::mat4 R = glm::mat4_cast(rotation);
						glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

						node.LocalTransform = T * R * S;
					},
					[&](const fastgltf::math::fmat4x4& matrix) {
						node.LocalTransform = glm::mat4(
							matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],
							matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],
							matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3],
							matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]
						);
					}
				}, gltfNode.transform);

				node.Children.clear();
				for (size_t childIdx : gltfNode.children) {
					node.Children.push_back(static_cast<uint32_t>(childIdx));
				}

				if (gltfNode.meshIndex.has_value()) {
					size_t meshIdx = gltfNode.meshIndex.value();
					if (meshIdx < meshToSubmeshes.size()) {
						node.Submeshes = meshToSubmeshes[meshIdx];
						ZN_MESH_LOG("Node '{}' has {} submeshes from mesh {}",
							node.Name, node.Submeshes.size(), meshIdx);
					}
				}

				ZN_MESH_LOG("Node {}: '{}', Children: {}, Submeshes: {}",
					nodeIndex, node.Name, node.Children.size(), node.Submeshes.size());
			}

			for (size_t nodeIndex = 0; nodeIndex < meshSource->m_Nodes.size(); nodeIndex++) {
				for (uint32_t childIndex : meshSource->m_Nodes[nodeIndex].Children) {
					if (childIndex < meshSource->m_Nodes.size()) {
						meshSource->m_Nodes[childIndex].Parent = static_cast<uint32_t>(nodeIndex);
					}
				}
			}

			uint32_t rootCount = 0;
			for (const auto& node : meshSource->m_Nodes) {
				if (node.Parent == UINT32_MAX) {
					rootCount++;
				}
			}

			ZN_MESH_LOG("Found {} root nodes", rootCount);
		}

		ProcessMaterials(meshSource, &asset, MeshFormat::GLTF);

		if (!meshSource->m_Vertices.empty())
			meshSource->m_VertexBuffer = VertexBuffer::Create(meshSource->m_Vertices.data(),
				static_cast<uint32_t>(meshSource->m_Vertices.size() * sizeof(Vertex)));

		if (!meshSource->m_Indices.empty())
			meshSource->m_IndexBuffer = IndexBuffer::Create(meshSource->m_Indices.data(),
				static_cast<uint32_t>(meshSource->m_Indices.size() * sizeof(uint32_t)));

		ZN_MESH_LOG("glTF Import complete: {} vertices, {} indices, {} submeshes, {} nodes",
			meshSource->m_Vertices.size(), meshSource->m_Indices.size(), meshSource->m_Submeshes.size(), meshSource->m_Nodes.size());

		const auto& bb = meshSource->m_BoundingBox;
		ZN_MESH_LOG("Mesh '{}' Bounds:\n  Min: ({}, {}, {})\n  Max: ({}, {}, {})\n  Size: ({}, {}, {})",
			meshSource->m_FilePath,
			bb.Min.x, bb.Min.y, bb.Min.z,
			bb.Max.x, bb.Max.y, bb.Max.z,
			bb.Max.x - bb.Min.x, bb.Max.y - bb.Min.y, bb.Max.z - bb.Min.z
		);

		return meshSource;
	}

	Ref<MeshSource> MeshImporter::ImportOBJ()
	{
		fastObjMesh* mesh = fast_obj_read(m_Path.string().c_str());
		if (!mesh)
		{
			ZN_CORE_ERROR_TAG("Mesh", "Failed to load OBJ file: {0}", m_Path.string());
			return nullptr;
		}

		Ref<MeshSource> meshSource = Ref<MeshSource>::Create();
		meshSource->m_FilePath = m_Path.string();

		meshSource->m_BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
		meshSource->m_BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;

		// Map to store unique vertices and their final index
		std::unordered_map<VertexKey, uint32_t, VertexKeyHash> uniqueVertices;

		for (uint32_t groupIndex = 0; groupIndex < mesh->group_count; groupIndex++)
		{
			const fastObjGroup& group = mesh->groups[groupIndex];

			Submesh& submesh = meshSource->m_Submeshes.emplace_back();
			submesh.BaseVertex = vertexCount;
			submesh.BaseIndex = indexCount;
			submesh.MaterialIndex = 0;
			submesh.MeshName = group.name ? group.name : "Group_" + std::to_string(groupIndex);

			uint32_t groupVertexCount = 0;

			ZN_MESH_LOG("Group {} '{}' has {} faces", groupIndex, submesh.MeshName, group.face_count);

			std::vector<uint32_t> submeshIndices;

			uint32_t groupStartTriangle = group.face_offset;
			uint32_t groupTriangleCount = group.face_count;

			ZN_MESH_LOG("Group {} triangle range: start={}, count={}", groupIndex, groupStartTriangle, groupTriangleCount);

			for (uint32_t localTriangleIndex = 0; localTriangleIndex < groupTriangleCount; localTriangleIndex++)
			{
				uint32_t globalTriangleIndex = groupStartTriangle + localTriangleIndex;

				std::array<uint32_t, 3> triangleIndices;

				for (int i = 0; i < 3; i++)
				{
					uint32_t indexPos = globalTriangleIndex * 3 + i;
					if (indexPos >= mesh->index_count) break;

					fastObjIndex objIndex = mesh->indices[indexPos];

					ZN_MESH_LOG("VertexKey: p={}, n={}, t={}", objIndex.p, objIndex.n, objIndex.t);

					Vertex vertex;

					if (objIndex.p > 0 && objIndex.p <= mesh->position_count)
					{
						vertex.Position = {
							mesh->positions[(objIndex.p - 1) * 3 + 0],
							mesh->positions[(objIndex.p - 1) * 3 + 1],
							mesh->positions[(objIndex.p - 1) * 3 + 2]
						};
					}

					if (objIndex.n > 0 && objIndex.n <= mesh->normal_count)
					{
						vertex.Normal = {
							mesh->normals[(objIndex.n - 1) * 3 + 0],
							mesh->normals[(objIndex.n - 1) * 3 + 1],
							mesh->normals[(objIndex.n - 1) * 3 + 2]
						};
					}

					if (objIndex.t > 0 && objIndex.t <= mesh->texcoord_count)
					{
						vertex.Texcoord = {
							mesh->texcoords[(objIndex.t - 1) * 2 + 0],
							1.0f - mesh->texcoords[(objIndex.t - 1) * 2 + 1]
						};
					}

					ZN_MESH_LOG("New Vertex: Pos=({}, {}, {}), Normal=({}, {}, {}), Tex=({}, {})",
						vertex.Position.x, vertex.Position.y, vertex.Position.z,
						vertex.Normal.x, vertex.Normal.y, vertex.Normal.z,
						vertex.Texcoord.x, vertex.Texcoord.y);

					meshSource->m_Vertices.push_back(vertex);
					triangleIndices[i] = static_cast<uint32_t>(meshSource->m_Vertices.size()) - 1;
					groupVertexCount++;

					meshSource->m_BoundingBox.Min = glm::min(meshSource->m_BoundingBox.Min, vertex.Position);
					meshSource->m_BoundingBox.Max = glm::max(meshSource->m_BoundingBox.Max, vertex.Position);
				}

				uint32_t i0 = triangleIndices[0];
				uint32_t i1 = triangleIndices[1];
				uint32_t i2 = triangleIndices[2];

				if (i0 != i1 && i1 != i2 && i0 != i2)
				{
					submeshIndices.push_back(i0);
					submeshIndices.push_back(i1);
					submeshIndices.push_back(i2);
				}
				else
				{
					ZN_MESH_LOG("Skipping degenerate triangle: {}, {}, {}", i0, i1, i2);
				}
			}

			if (!ValidateIndices(submeshIndices, static_cast<uint32_t>(meshSource->m_Vertices.size()), submesh.MeshName))
			{
				ZN_MESH_ERROR("Skipping submesh '{}' due to invalid indices", submesh.MeshName);
				meshSource->m_Submeshes.pop_back();
				continue;
			}

			meshSource->m_Indices.insert(meshSource->m_Indices.end(), submeshIndices.begin(), submeshIndices.end());

			submesh.VertexCount = static_cast<uint32_t>(meshSource->m_Vertices.size()) - submesh.BaseVertex;
			submesh.IndexCount = static_cast<uint32_t>(submeshIndices.size());

			vertexCount += submesh.VertexCount;
			indexCount += submesh.IndexCount;

			submesh.BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
			submesh.BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

			for (uint32_t vi = submesh.BaseVertex; vi < submesh.BaseVertex + submesh.VertexCount; vi++)
			{
				const auto& pos = meshSource->m_Vertices[vi].Position;
				submesh.BoundingBox.Min = glm::min(submesh.BoundingBox.Min, pos);
				submesh.BoundingBox.Max = glm::max(submesh.BoundingBox.Max, pos);
			}

			ZN_MESH_LOG("OBJ Submesh '{}': {} vertices, {} indices", submesh.MeshName, submesh.VertexCount, submesh.IndexCount);
		}

		ProcessMaterials(meshSource, mesh, MeshFormat::OBJ);

		if (!meshSource->m_Vertices.empty())
			meshSource->m_VertexBuffer = VertexBuffer::Create(meshSource->m_Vertices.data(),
				static_cast<uint32_t>(meshSource->m_Vertices.size() * sizeof(Vertex)));

		if (!meshSource->m_Indices.empty())
			meshSource->m_IndexBuffer = IndexBuffer::Create(meshSource->m_Indices.data(),
				static_cast<uint32_t>(meshSource->m_Indices.size() * sizeof(uint32_t)));

		ZN_MESH_LOG("OBJ Import complete: {} vertices, {} indices, {} submeshes",
			meshSource->m_Vertices.size(), meshSource->m_Indices.size(), meshSource->m_Submeshes.size());

		const auto& bb = meshSource->m_BoundingBox;
		ZN_MESH_LOG("Mesh '{}' Bounds:\n  Min: ({}, {}, {})\n  Max: ({}, {}, {})\n  Size: ({}, {}, {})",
			meshSource->m_FilePath,
			bb.Min.x, bb.Min.y, bb.Min.z,
			bb.Max.x, bb.Max.y, bb.Max.z,
			bb.Max.x - bb.Min.x, bb.Max.y - bb.Min.y, bb.Max.z - bb.Min.z
		);

		fast_obj_destroy(mesh);
		return meshSource;
	}

	void MeshImporter::ProcessMaterials(Ref<MeshSource> meshSource, void* scene, MeshFormat format)
	{
		switch (format)
		{
			case MeshFormat::FBX:
			{
				ufbx_scene* fbxScene = static_cast<ufbx_scene*>(scene);
				meshSource->m_Materials.resize(fbxScene->materials.count);

				for (size_t i = 0; i < fbxScene->materials.count; i++)
				{
					meshSource->m_Materials[i] = AssetHandle{0};
				}
				break;
			}

			case MeshFormat::GLTF:
			case MeshFormat::GLB:
			{
				fastgltf::Asset* gltfAsset = static_cast<fastgltf::Asset*>(scene);
				meshSource->m_Materials.resize(gltfAsset->materials.size());

				ZN_CORE_INFO("Processing {} GLTF materials", gltfAsset->materials.size());

				for (size_t i = 0; i < gltfAsset->materials.size(); i++)
				{
					Ref<MaterialAsset> materialAsset = CreateMaterialFromGLTF(*gltfAsset, i);
					if (materialAsset)
					{
						AssetHandle handle = AssetManager::AddMemoryOnlyAsset(materialAsset);
						materialAsset->Handle = handle;
						meshSource->m_Materials[i] = handle;

						ZN_MESH_LOG("Created material[{}] with handle {}", i, static_cast<uint64_t>(handle));
					}
					else
					{
						meshSource->m_Materials[i] = AssetHandle{0};
						ZN_MESH_LOG("Failed to create material[{}], using default", i);
					}
				}
				break;


				/*for (size_t i = 0; i < gltfAsset->materials.size(); i++)
				{
					meshSource->m_Materials[i] = AssetHandle{ 0 };
				}
				break;*/
			}

			case MeshFormat::OBJ:
			{
				fastObjMesh* objMesh = static_cast<fastObjMesh*>(scene);
				meshSource->m_Materials.resize(objMesh->material_count);

				for (uint32_t i = 0; i < objMesh->material_count; i++)
				{
					meshSource->m_Materials[i] = AssetHandle{0};
				}
				break;
			}

			case MeshFormat::Unknown:
			default:
				ZN_CORE_WARN_TAG("Mesh", "Unknown mesh format for material processing");
				break;
		}

		if (meshSource->m_Materials.empty())
		{
			meshSource->m_Materials.push_back(AssetHandle{0});
		}
	}

	Ref<MaterialAsset> MeshImporter::CreateMaterialFromGLTF(const fastgltf::Asset& asset, size_t materialIndex)
	{
		if (materialIndex >= asset.materials.size())
		{
			ZN_CORE_ERROR_TAG("Mesh", "Invalid material index: {}", materialIndex);
			return nullptr;
		}

		const auto& gltfMaterial = asset.materials[materialIndex];
		bool isTransparent = gltfMaterial.alphaMode != fastgltf::AlphaMode::Opaque;

		ZN_MESH_LOG("Creating material[{}]: '{}' (transparent: {})", materialIndex, gltfMaterial.name, isTransparent);

		auto shaderLibrary = Renderer::GetShaderLibrary();
		if (!shaderLibrary)
		{
			ZN_CORE_ERROR_TAG("Mesh", "Shader library is null");
			return nullptr;
		}

		const std::string shaderName = isTransparent ? "PBR_TransparentMesh" : "PBR_StaticMesh";
		auto shader = shaderLibrary->Get(shaderName);
		if (!shader)
		{
			ZN_CORE_ERROR_TAG("Mesh", "Required shader '{}' not found in shader library", shaderName);
			return nullptr;
		}

		ZN_MESH_LOG("Found required shader: {}", shaderName);

		Ref<MaterialAsset> materialAsset = Ref<MaterialAsset>::Create(isTransparent);
		if (!materialAsset)
		{
			ZN_CORE_ERROR_TAG("Mesh", "Failed to create MaterialAsset");
			return nullptr;
		}

		ZN_MESH_LOG("MaterialAsset created successfully");

		auto internalMaterial = materialAsset->GetMaterial();
		if (!internalMaterial)
		{
			ZN_CORE_ERROR_TAG("Mesh", "MaterialAsset has null internal Material");
			return nullptr;
		}

		ZN_MESH_LOG("Internal Material is valid");

		/*try
		{
			glm::vec3 testColor(1.0f, 0.0f, 1.0f);
			materialAsset->SetAlbedoColor(testColor);
			ZN_MESH_LOG("Successfully set test albedo color");

			const auto& pbr = gltfMaterial.pbrData;
			glm::vec3 baseColor(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2]);
			materialAsset->SetAlbedoColor(baseColor);
			ZN_MESH_LOG(" - Base Color: ({:.2f}, {:.2f}, {:.2f})", baseColor.x, baseColor.y, baseColor.z);
		}
		catch (const std::exception& e)
		{
			ZN_CORE_ERROR_TAG("Mesh", "Exception setting albedo color: {}", e.what());
			return nullptr;
		}
		catch (...)
		{
			ZN_CORE_ERROR_TAG("Mesh", "Unknown exception setting albedo color");
			return nullptr;
		}

		if (!isTransparent)
		{
			try
			{
				const auto& pbr = gltfMaterial.pbrData;
				materialAsset->SetMetalness(pbr.metallicFactor);
				materialAsset->SetRoughness(pbr.roughnessFactor);
				ZN_MESH_LOG(" - Metallic: {:.2f}, Roughness: {:.2f}", pbr.metallicFactor, pbr.roughnessFactor);
			}
			catch (...)
			{
				ZN_CORE_ERROR_TAG("Mesh", "Failed to set PBR properties");
			}
		}*/

		ZN_MESH_LOG("Material[{}] created successfully", materialIndex);
		return materialAsset;
	}


	AssetHandle MeshImporter::ProcessGLTFTexture(const fastgltf::Asset& asset, size_t textureIndex, const std::string& semanticName)
	{
		if (textureIndex >= asset.textures.size())
		{
			ZN_CORE_ERROR_TAG("Mesh", "Invalid texture index: {}", textureIndex);
			return AssetHandle{0};
		}

		const auto& gltfTexture = asset.textures[textureIndex];

		if (!gltfTexture.imageIndex.has_value())
		{
			ZN_CORE_ERROR_TAG("Mesh", "Texture has no image reference");
			return AssetHandle{0};
		}

		return LoadImageFromGLTF(asset, gltfTexture.imageIndex.value(), semanticName);
	}

	AssetHandle MeshImporter::LoadImageFromGLTF(const fastgltf::Asset& asset, size_t imageIndex, const std::string& debugName)
	{
		if (imageIndex >= asset.images.size())
		{
			ZN_CORE_ERROR_TAG("Mesh", "Invalid image index: {}", imageIndex);
			return AssetHandle{0};
		}

		const auto& gltfImage = asset.images[imageIndex];
		AssetHandle resultHandle{0};

		if (auto uri = std::get_if<fastgltf::sources::URI>(&gltfImage.data))
		{
			std::filesystem::path imagePath = m_Path.parent_path() / uri->uri.path();

			ZN_MESH_LOG("Loading {} texture from: {}", debugName, imagePath.string());

			if (!std::filesystem::exists(imagePath))
			{
				ZN_CORE_ERROR_TAG("Mesh", "Image file not found: {}", imagePath.string());
				return AssetHandle{0};
			}

			TextureSpecification spec;
			spec.Width = 0;
			spec.Height = 0;
			spec.Format = ImageFormat::RGBA;
			spec.GenerateMips = true;

			Ref<Texture2D> texture = Texture2D::Create(spec, imagePath);
			if (!texture)
			{
				ZN_CORE_ERROR_TAG("Mesh", "Failed to load texture: {}", imagePath.string());
				return AssetHandle{0};
			}

			AssetHandle handle = AssetManager::AddMemoryOnlyAsset(texture);
			resultHandle = handle;

			ZN_MESH_LOG("Loaded {} texture: {} ({}x{})", debugName, imagePath.filename().string(),
					   texture->GetWidth(), texture->GetHeight());
		}
		else if (auto vector = std::get_if<fastgltf::sources::Vector>(&gltfImage.data))
		{
			ZN_MESH_LOG("Loading {} texture from embedded data ({} bytes)", debugName, vector->bytes.size());
			Buffer imageBuffer = Buffer::Copy(vector->bytes.data(), vector->bytes.size());

			TextureSpecification spec;
			spec.Format = ImageFormat::RGBA;
			spec.GenerateMips = true;

			Ref<Texture2D> texture = Texture2D::Create(spec, imageBuffer);
			if (!texture)
			{
				ZN_CORE_ERROR_TAG("Mesh", "Failed to create texture from embedded data");
				return AssetHandle{0};
			}

			AssetHandle handle = AssetManager::AddMemoryOnlyAsset(texture);
			resultHandle = handle;

			ZN_MESH_LOG("Created {} texture from embedded data ({}x{})", debugName,
					   texture->GetWidth(), texture->GetHeight());
		}
		else if (auto bufferView = std::get_if<fastgltf::sources::BufferView>(&gltfImage.data))
		{
			ZN_MESH_LOG("Loading {} texture from buffer view", debugName);

			if (bufferView->bufferViewIndex >= asset.bufferViews.size())
			{
				ZN_CORE_ERROR_TAG("Mesh", "Invalid buffer view index: {}", bufferView->bufferViewIndex);
				return AssetHandle{0};
			}

			const auto& view = asset.bufferViews[bufferView->bufferViewIndex];
			const auto& buffer = asset.buffers[view.bufferIndex];

			if (auto array = std::get_if<fastgltf::sources::Array>(&buffer.data))
			{
				const uint8_t* data = reinterpret_cast<const uint8_t*>(array->bytes.data()) + view.byteOffset;
				size_t size = view.byteLength;

				Buffer imageBuffer = Buffer::Copy(data, static_cast<uint32_t>(size));

				TextureSpecification spec;
				spec.Format = ImageFormat::RGBA;
				spec.GenerateMips = true;

				Ref<Texture2D> texture = Texture2D::Create(spec, imageBuffer);
				if (!texture)
				{
					ZN_CORE_ERROR_TAG("Mesh", "Failed to create texture from buffer view");
					return AssetHandle{0};
				}

				AssetHandle handle = AssetManager::AddMemoryOnlyAsset(texture);
				resultHandle = handle;

				ZN_MESH_LOG("Created {} texture from buffer view ({}x{})", debugName,
						   texture->GetWidth(), texture->GetHeight());
			}
			else
			{
				ZN_CORE_ERROR_TAG("Mesh", "Unsupported buffer data source for texture");
				return AssetHandle{0};
			}
		}
		else
		{
			ZN_CORE_ERROR_TAG("Mesh", "Unsupported image data source");
			return AssetHandle{0};
		}

		return resultHandle;
	}

	void MeshImporter::DebugMaterialLoading(const Ref<MeshSource>& meshSource)
	{
		ZN_CORE_INFO("Mesh has {} materials", meshSource->m_Materials.size());

		for (size_t i = 0; i < meshSource->m_Materials.size(); i++)
		{
			AssetHandle materialHandle = meshSource->m_Materials[i];
			ZN_MESH_LOG("Material[{}] Handle: {}", i, static_cast<uint64_t>(materialHandle));

			if (materialHandle)
			{
				Ref<MaterialAsset> mat = AssetManager::GetAsset<MaterialAsset>(materialHandle);
				if (mat)
				{
					ZN_MESH_LOG("Material[{}] loaded successfully:", i);

					if (Ref<Texture2D> albedoTex = mat->GetAlbedoMap())
					{
						ZN_MESH_LOG(" - Albedo texture loaded: {}x{}",
								   albedoTex->GetWidth(), albedoTex->GetHeight());
					}
					else
					{
						ZN_MESH_LOG(" - No albedo texture");
					}

					if (mat->IsUsingNormalMap())
					{
						if (Ref<Texture2D> normalTex = mat->GetNormalMap())
						{
							ZN_MESH_LOG(" - Normal map loaded: {}x{}",
									   normalTex->GetWidth(), normalTex->GetHeight());
						}
					}
					else
					{
						ZN_MESH_LOG(" - No normal map");
					}

					if (Ref<Texture2D> metallicTex = mat->GetMetalnessMap())
					{
						ZN_MESH_LOG(" - Metallic map loaded: {}x{}",
								   metallicTex->GetWidth(), metallicTex->GetHeight());
					}
					else
					{
						ZN_MESH_LOG(" - No metallic map");
					}

					if (Ref<Texture2D> roughnessTex = mat->GetRoughnessMap())
					{
						ZN_MESH_LOG(" - Roughness map loaded: {}x{}",
								   roughnessTex->GetWidth(), roughnessTex->GetHeight());
					}
					else
					{
						ZN_MESH_LOG(" - No roughness map");
					}

					ZN_MESH_LOG(" - Albedo Color: ({:.2f}, {:.2f}, {:.2f})",
							   mat->GetAlbedoColor().x, mat->GetAlbedoColor().y, mat->GetAlbedoColor().z);
					ZN_MESH_LOG(" - Metalness: {:.2f}", mat->GetMetalness());
					ZN_MESH_LOG(" - Roughness: {:.2f}", mat->GetRoughness());
					ZN_MESH_LOG(" - Emission: {:.2f}", mat->GetEmission());
				}
				else
				{
					ZN_MESH_LOG("Material[{}] failed to load from asset manager", i);
				}
			}
			else
			{
				ZN_MESH_LOG("Material[{}] has null handle", i);
			}
		}

		for (size_t submeshIndex = 0; submeshIndex < meshSource->m_Submeshes.size(); submeshIndex++)
		{
			const Submesh& submesh = meshSource->m_Submeshes[submeshIndex];
			uint32_t materialIndex = submesh.MaterialIndex;

			if (materialIndex < meshSource->m_Materials.size())
			{
				AssetHandle materialHandle = meshSource->m_Materials[materialIndex];
				ZN_CORE_INFO("Submesh[{}] '{}' uses Material[{}] (Handle: {})",
						   submeshIndex, submesh.MeshName, materialIndex,
						   static_cast<uint64_t>(materialHandle));
			}
			else
			{
				ZN_CORE_INFO("Submesh[{}] has invalid material index: {}", submeshIndex, materialIndex);
			}
		}
	}

	AssetHandle MeshImporter::CreateMaterialFromTexture(const std::string& texturePath, const std::string& name)
	{
		ZN_MESH_LOG("Texture reference found but PBR materials disabled: {0}", texturePath);
		return AssetHandle{0};
	}

	glm::mat4 MeshImporter::ToGLMMat4(const float* matrix)
	{
		return glm::mat4(
			matrix[0], matrix[1], matrix[2], matrix[3],
			matrix[4], matrix[5], matrix[6], matrix[7],
			matrix[8], matrix[9], matrix[10], matrix[11],
			matrix[12], matrix[13], matrix[14], matrix[15]
		);
	}

	glm::vec3 MeshImporter::ToGLMVec3(const float* vec)
	{
		return glm::vec3(vec[0], vec[1], vec[2]);
	}

	glm::quat MeshImporter::ToGLMQuat(const float* quat)
	{
		return glm::quat(quat[3], quat[0], quat[1], quat[2]);
	}

}