#pragma once

#include "Zenith/Core/Base.hpp"
#include "Zenith/Core/Ref.hpp"

#include "Zenith/Renderer/Mesh.hpp"

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

		void BeginScene(const glm::mat4& viewProjection);
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

	private:
		Ref<Shader> m_MeshShader;
		Ref<Pipeline> m_Pipeline;
		Ref<RenderPass> m_RenderPass;
		Ref<Framebuffer> m_Framebuffer;
		Ref<Material> m_Material;
		Ref<RenderCommandBuffer> m_CommandBuffer;
		Ref<VertexBuffer> m_TransformBuffer;
		Ref<VertexBuffer> m_ColorBuffer;

		glm::mat4 m_ViewProjectionMatrix;
		bool m_SceneActive = false;

		std::unordered_map<MeshSource*, Ref<StaticMesh>> m_CachedStaticMeshes;
	};

}
