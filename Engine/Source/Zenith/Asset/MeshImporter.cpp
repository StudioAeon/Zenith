#include "znpch.hpp"
#include "MeshImporter.hpp"

#include "Zenith/Asset/AssetManager.hpp"
#include "Zenith/Renderer/Renderer.hpp"
#include "Zenith/Math/Math.hpp"

#include <ufbx.h>
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fast_obj.h>

#include <glm/detail/type_quat.hpp>

namespace Zenith {

#define MESH_DEBUG_LOG 0

#if MESH_DEBUG_LOG
#define ZN_MESH_LOG(...) ZN_CORE_TRACE_TAG("Mesh", __VA_ARGS__)
#define ZN_MESH_ERROR(...) ZN_CORE_ERROR_TAG("Mesh", __VA_ARGS__)
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

		if (extension == ".fbx") return MeshFormat::FBX;
		if (extension == ".gltf") return MeshFormat::GLTF;
		if (extension == ".glb") return MeshFormat::GLB;
		if (extension == ".obj") return MeshFormat::OBJ;

		return MeshFormat::Unknown;
	}

	Ref<MeshSource> MeshImporter::ImportToMeshSource()
	{
		ZN_CORE_INFO_TAG("Mesh", "Loading mesh: {0}", m_Path.string());

		switch (m_Format)
		{
			case MeshFormat::FBX:
				return ImportFBX();
			case MeshFormat::GLTF:
			case MeshFormat::GLB:
				return ImportGLTF();
			case MeshFormat::OBJ:
				return ImportOBJ();
			default:
				ZN_CORE_ERROR_TAG("Mesh", "Unsupported mesh format: {0}", m_Path.string());
				return nullptr;
		}
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

			for (size_t fi = 0; fi < mesh->faces.count; fi++)
			{
				ufbx_face face = mesh->faces.data[fi];

				for (uint32_t tri = 0; tri < face.num_indices - 2; tri++)
				{
					Index index;
					index.V1 = static_cast<uint32_t>(mesh->vertex_indices.data[face.index_begin + 0]) + submesh.BaseVertex;
					index.V2 = static_cast<uint32_t>(mesh->vertex_indices.data[face.index_begin + tri + 1]) + submesh.BaseVertex;
					index.V3 = static_cast<uint32_t>(mesh->vertex_indices.data[face.index_begin + tri + 2]) + submesh.BaseVertex;

					meshSource->m_Indices.push_back(index);

					meshSource->m_TriangleCache[i].emplace_back(
						meshSource->m_Vertices[index.V1],
						meshSource->m_Vertices[index.V2],
						meshSource->m_Vertices[index.V3]
					);
				}
			}

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
		}

		ProcessMaterials(meshSource, scene, MeshFormat::FBX);

		if (!meshSource->m_Vertices.empty())
			meshSource->m_VertexBuffer = VertexBuffer::Create(meshSource->m_Vertices.data(),
				static_cast<uint32_t>(meshSource->m_Vertices.size() * sizeof(Vertex)));

		if (!meshSource->m_Indices.empty())
			meshSource->m_IndexBuffer = IndexBuffer::Create(meshSource->m_Indices.data(),
				static_cast<uint32_t>(meshSource->m_Indices.size() * sizeof(Index)));

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

		auto assetResult = parser.loadGltf(data.get(), m_Path.parent_path());
		if (assetResult.error() != fastgltf::Error::None)
		{
			ZN_CORE_ERROR_TAG("Mesh", "Failed to parse glTF file: {0}", m_Path.string());
			return nullptr;
		}

		fastgltf::Asset& asset = assetResult.get();

		Ref<MeshSource> meshSource = Ref<MeshSource>::Create();
		meshSource->m_FilePath = m_Path.string();

		meshSource->m_BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
		meshSource->m_BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;

		for (size_t meshIndex = 0; meshIndex < asset.meshes.size(); meshIndex++)
		{
			const auto& mesh = asset.meshes[meshIndex];

			for (size_t primIndex = 0; primIndex < mesh.primitives.size(); primIndex++)
			{
				const auto& primitive = mesh.primitives[primIndex];

				if (primitive.type != fastgltf::PrimitiveType::Triangles)
					continue;

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

					meshSource->m_Vertices.push_back(vertex);

					meshSource->m_BoundingBox.Min = glm::min(meshSource->m_BoundingBox.Min, vertex.Position);
					meshSource->m_BoundingBox.Max = glm::max(meshSource->m_BoundingBox.Max, vertex.Position);
				}

				if (primitive.indicesAccessor)
				{
					const auto& indexAccessor = asset.accessors[*primitive.indicesAccessor];
					submesh.IndexCount = static_cast<uint32_t>(indexAccessor.count);

					std::vector<uint32_t> indices;
					fastgltf::iterateAccessor<std::uint32_t>(asset, indexAccessor, [&](std::uint32_t index) {
						indices.push_back(index + submesh.BaseVertex);
					});

					for (size_t i = 0; i < indices.size(); i += 3)
					{
						Index index;
						index.V1 = indices[i];
						index.V2 = indices[i + 1];
						index.V3 = indices[i + 2];
						meshSource->m_Indices.push_back(index);

						meshSource->m_TriangleCache[meshIndex].emplace_back(
							meshSource->m_Vertices[index.V1],
							meshSource->m_Vertices[index.V2],
							meshSource->m_Vertices[index.V3]
						);
					}
				}

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
			}
		}

		ProcessMaterials(meshSource, &asset, MeshFormat::GLTF);

		if (!meshSource->m_Vertices.empty())
			meshSource->m_VertexBuffer = VertexBuffer::Create(meshSource->m_Vertices.data(),
				static_cast<uint32_t>(meshSource->m_Vertices.size() * sizeof(Vertex)));

		if (!meshSource->m_Indices.empty())
			meshSource->m_IndexBuffer = IndexBuffer::Create(meshSource->m_Indices.data(),
				static_cast<uint32_t>(meshSource->m_Indices.size() * sizeof(Index)));

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

		// Each group becomes a submesh
		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;

		for (uint32_t groupIndex = 0; groupIndex < mesh->group_count; groupIndex++)
		{
			const fastObjGroup& group = mesh->groups[groupIndex];

			Submesh& submesh = meshSource->m_Submeshes.emplace_back();
			submesh.BaseVertex = vertexCount;
			submesh.BaseIndex = indexCount;
			submesh.MaterialIndex = 0;
			submesh.MeshName = group.name ? group.name : "Group_" + std::to_string(groupIndex);

			uint32_t groupVertexCount = 0;
			uint32_t groupIndexCount = 0;

			for (uint32_t faceIndex = group.face_offset; faceIndex < group.face_offset + group.face_count; faceIndex++)
			{
				groupIndexCount += 3;
			}

			std::unordered_map<uint64_t, uint32_t> vertexMap;
			std::vector<uint32_t> faceIndices;

			for (uint32_t faceIndex = group.face_offset; faceIndex < group.face_offset + group.face_count; faceIndex++)
			{
				for (int i = 0; i < 3; i++)
				{
					fastObjIndex objIndex = mesh->indices[faceIndex * 3 + i];
					uint64_t vertexHash = ((uint64_t)objIndex.p << 32) | ((uint64_t)objIndex.t << 16) | (uint64_t)objIndex.n;

					if (vertexMap.find(vertexHash) == vertexMap.end())
					{
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

						meshSource->m_Vertices.push_back(vertex);
						vertexMap[vertexHash] = groupVertexCount++;

						meshSource->m_BoundingBox.Min = glm::min(meshSource->m_BoundingBox.Min, vertex.Position);
						meshSource->m_BoundingBox.Max = glm::max(meshSource->m_BoundingBox.Max, vertex.Position);
					}

					faceIndices.push_back(vertexMap[vertexHash] + submesh.BaseVertex);
				}
			}

			for (size_t i = 0; i < faceIndices.size(); i += 3)
			{
				Index index;
				index.V1 = faceIndices[i];
				index.V2 = faceIndices[i + 1];
				index.V3 = faceIndices[i + 2];
				meshSource->m_Indices.push_back(index);

				meshSource->m_TriangleCache[groupIndex].emplace_back(
					meshSource->m_Vertices[index.V1],
					meshSource->m_Vertices[index.V2],
					meshSource->m_Vertices[index.V3]
				);
			}

			submesh.VertexCount = groupVertexCount;
			submesh.IndexCount = groupIndexCount;

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
		}

		ProcessMaterials(meshSource, mesh, MeshFormat::OBJ);

		if (!meshSource->m_Vertices.empty())
			meshSource->m_VertexBuffer = VertexBuffer::Create(meshSource->m_Vertices.data(),
				static_cast<uint32_t>(meshSource->m_Vertices.size() * sizeof(Vertex)));

		if (!meshSource->m_Indices.empty())
			meshSource->m_IndexBuffer = IndexBuffer::Create(meshSource->m_Indices.data(),
				static_cast<uint32_t>(meshSource->m_Indices.size() * sizeof(Index)));

		fast_obj_destroy(mesh);
		return meshSource;
	}

	void MeshImporter::ProcessMaterials(Ref<MeshSource> meshSource, void* scene, MeshFormat format)
	{
		// just count materials without creating complex material assets
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

				for (size_t i = 0; i < gltfAsset->materials.size(); i++)
				{
					meshSource->m_Materials[i] = AssetHandle{0};
				}
				break;
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
		return glm::quat(quat[3], quat[0], quat[1], quat[2]); // w, x, y, z
	}

}