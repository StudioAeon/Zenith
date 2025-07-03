#pragma once

#include "Zenith/Core/Base.hpp"
#include "Zenith/Core/Ref.hpp"

#include "Zenith/Renderer/Mesh.hpp"
#include "Zenith/Renderer/MaterialAsset.hpp"
#include "Zenith/Renderer/UniformBuffer.hpp"

#include "Zenith/Renderer/Pipeline.hpp"
#include "Zenith/Renderer/Shader.hpp"
#include "Zenith/Renderer/RenderCommandBuffer.hpp"
#include "Zenith/Renderer/Framebuffer.hpp"
#include "Zenith/Renderer/RenderPass.hpp"
#include "Zenith/Renderer/VertexBuffer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanImage.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanMaterial.hpp"

#include <imgui/backends/imgui_impl_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>

#include "imgui.h"

namespace Zenith {

	 class MeshRenderer
	 {
	 public:
		 MeshRenderer();
		 ~MeshRenderer();

		 void Initialize();
		 void Shutdown();

		 void BeginScene(const glm::mat4& viewProjection, const glm::vec3& cameraPosition = glm::vec3(0.0f));
		 void DrawMesh(Ref<MeshSource> meshSource, const glm::mat4& transform = glm::mat4(1.0f));
		 void EndScene();

		 Ref<Image2D> GetImage(uint32_t attachmentIndex = 0) const
		 {
			 return m_Framebuffer ? m_Framebuffer->GetImage(attachmentIndex) : nullptr;
		 }

		 std::unordered_map<void*, VkDescriptorSet> m_TextureDescriptorCache;
		 ImVec2 m_LastViewportSize = {0, 0};

		 ImTextureID GetTextureImGuiID(Ref<Image2D> image);
		 void ClearTextureCache();

	 private:
		 void CreatePipeline();
		 void CreateRenderPass();
		 Ref<StaticMesh> GetOrCreateStaticMesh(Ref<MeshSource> meshSource);

		 void TraverseNodeHierarchy(Ref<MeshSource> meshSource, Ref<StaticMesh> staticMesh,
			 const std::vector<MeshNode>& nodes, uint32_t nodeIndex, const glm::mat4& parentTransform);

		 void RenderSubmesh(Ref<MeshSource> meshSource, Ref<StaticMesh> staticMesh,
			 uint32_t submeshIndex, const glm::mat4& transform);

		 Ref<MaterialAsset> GetMaterialForSubmesh(Ref<MeshSource> meshSource, uint32_t materialIndex);
		 Ref<Material> GetOrCreatePBRMaterial(Ref<MaterialAsset> materialAsset);
		 void UpdateMaterialUniforms(Ref<Material> material, Ref<MaterialAsset> materialAsset);
		 void SetMaterialTextures(Ref<Material> material, Ref<MaterialAsset> materialAsset);

	 private:
		 Ref<Shader> m_MeshShader;
		 Ref<Pipeline> m_Pipeline;
		 Ref<RenderPass> m_RenderPass;
		 Ref<Framebuffer> m_Framebuffer;
		 Ref<RenderCommandBuffer> m_CommandBuffer;

		 Ref<UniformBuffer> m_MaterialUniformBuffer;
		 Ref<UniformBuffer> m_CameraUniformBuffer;

		 glm::mat4 m_ViewProjectionMatrix;
		 glm::vec3 m_CameraPosition;
		 bool m_SceneActive = false;

		 std::unordered_map<MeshSource*, Ref<StaticMesh>> m_CachedStaticMeshes;
		 std::unordered_map<AssetHandle, Ref<Material>> m_MaterialCache;

		 struct MaterialUniforms
		 {
			 glm::vec3 u_AlbedoColor;
			 float u_Metalness;
			 float u_Roughness;
			 float u_Emission;
			 int u_UseNormalMap;
			 float _Padding2;
		 };

		 struct CameraUniforms
		 {
			 glm::mat4 u_ViewProjection;
			 glm::vec3 CameraPosition;
			 float _Padding;
		 };

		 struct PBRPushConstants
		 {
			 glm::mat4 u_Transform;
		 };

		 static_assert(sizeof(MaterialUniforms) == 32, "MaterialUniforms size mismatch with HLSL");
		 static_assert(sizeof(CameraUniforms) == 80, "CameraUniforms size mismatch with HLSL");
		 static_assert(sizeof(PBRPushConstants) == 64, "PBRPushConstants size mismatch with HLSL");
	 };

}
