#include "znpch.hpp"
#include "MeshRenderer.hpp"

#include "Zenith/Renderer/Renderer.hpp"

#include "Zenith/Core/Application.hpp"
#include "Zenith/Asset/AssetManager.hpp"

#include <glm/gtc/matrix_inverse.hpp>

namespace Zenith {

	MeshRenderer::MeshRenderer()
	{
	}

	MeshRenderer::~MeshRenderer()
	{
		Shutdown();
	}

	void MeshRenderer::Initialize()
	{
		m_CommandBuffer = RenderCommandBuffer::Create();

		FramebufferSpecification fbSpec;
		fbSpec.SwapChainTarget = false;
		fbSpec.Width = 1920;
		fbSpec.Height = 1080;
		fbSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
		fbSpec.DepthClearValue = 1.0f;
		fbSpec.ClearColorOnLoad = true;
		fbSpec.ClearDepthOnLoad = true;
		fbSpec.DebugName = "MeshRenderer-SwapChain";
		fbSpec.Attachments = FramebufferAttachmentSpecification{
			{ ImageFormat::RGBA },
			{ ImageFormat::DEPTH24STENCIL8 }
		};
		m_Framebuffer = Framebuffer::Create(fbSpec);

		m_MeshShader = Renderer::GetShaderLibrary()->Get("BasicMesh");

		m_TransformBuffer = VertexBuffer::Create(sizeof(glm::mat4));
		glm::mat4 identity(1.0f);
		m_TransformBuffer->SetData(&identity, sizeof(glm::mat4));

		CreatePipeline();
		CreateRenderPass();

		m_Material = Material::Create(m_MeshShader);
	}

	void MeshRenderer::Shutdown()
	{
		m_Pipeline = nullptr;
		m_RenderPass = nullptr;
		m_MeshShader = nullptr;
		m_Material = nullptr;
		m_CommandBuffer = nullptr;
		m_TransformBuffer = nullptr;
		m_CachedStaticMeshes.clear();
	}

	void MeshRenderer::CreatePipeline()
	{
		VertexBufferLayout vertexLayout = {
			{ ShaderDataType::Float3, "Position" },
			{ ShaderDataType::Float3, "Normal" },
			{ ShaderDataType::Float3, "Tangent" },
			{ ShaderDataType::Float3, "Binormal" },
			{ ShaderDataType::Float2, "TexCoord" }
		};

		PipelineSpecification pipelineSpec;
		pipelineSpec.DebugName = "MeshRenderer-Pipeline";
		pipelineSpec.Shader = m_MeshShader;
		pipelineSpec.TargetFramebuffer = m_Framebuffer;
		pipelineSpec.Layout = vertexLayout;
		pipelineSpec.BackfaceCulling = false;
		pipelineSpec.DepthTest = true;
		pipelineSpec.DepthWrite = true;
		pipelineSpec.Wireframe = false;
		pipelineSpec.DepthOperator = DepthCompareOperator::LessOrEqual;
		pipelineSpec.Topology = PrimitiveTopology::Triangles;

		m_Pipeline = Pipeline::Create(pipelineSpec);
	}

	void MeshRenderer::CreateRenderPass()
	{
		RenderPassSpecification renderPassSpec;
		renderPassSpec.DebugName = "MeshRenderer-RenderPass";
		renderPassSpec.TargetPipeline = m_Pipeline;
		renderPassSpec.MarkerColor = { 0.2f, 0.8f, 0.2f, 1.0f };

		m_RenderPass = RenderPass::Create(renderPassSpec);
	}

	void MeshRenderer::BeginScene(const glm::mat4& viewProjection, const glm::vec3& cameraPosition)
	{
		m_ViewProjectionMatrix = viewProjection;
		m_CameraPosition = cameraPosition;
		m_SceneActive = true;

		m_CommandBuffer->Begin();

		// Since we're using an offscreen framebuffer (SwapChainTarget = false),
		// we need to begin our own render pass
		Renderer::BeginRenderPass(m_CommandBuffer, m_RenderPass, true);
	}

	Ref<StaticMesh> MeshRenderer::GetOrCreateStaticMesh(Ref<MeshSource> meshSource)
	{
		MeshSource* key = meshSource.Raw();
		auto it = m_CachedStaticMeshes.find(key);
		if (it != m_CachedStaticMeshes.end())
			return it->second;

		AssetHandle handle = meshSource->Handle;
		Ref<StaticMesh> staticMesh = Ref<StaticMesh>::Create(handle);
		m_CachedStaticMeshes[key] = staticMesh;

		return staticMesh;
	}

	void MeshRenderer::DrawMesh(Ref<MeshSource> meshSource, const glm::mat4& transform)
	{
		if (!m_SceneActive || !meshSource)
			return;

		Ref<StaticMesh> staticMesh = GetOrCreateStaticMesh(meshSource);

		const auto& nodes = meshSource->GetNodes();
		if (!nodes.empty()) {
			bool foundRoot = false;
			for (uint32_t i = 0; i < nodes.size(); i++) {
				if (nodes[i].IsRoot()) {
					TraverseNodeHierarchy(meshSource, staticMesh, nodes, i, transform);
					foundRoot = true;
				}
			}
			if (!foundRoot) {
				TraverseNodeHierarchy(meshSource, staticMesh, nodes, 0, transform);
			}
		} else {
			const auto& submeshes = meshSource->GetSubmeshes();
			for (uint32_t i = 0; i < submeshes.size(); i++) {
				const auto& submesh = submeshes[i];

				struct MeshPushConstants {
					alignas(16) glm::mat4 model;
					alignas(16) glm::mat4 viewProjection;
					alignas(16) glm::mat4 normalMatrix;
					alignas(16) glm::vec4 cameraPosition;
				};

				glm::mat4 modelMatrix = transform * submesh.Transform;
				MeshPushConstants pushConstants;
				pushConstants.model = modelMatrix;
				pushConstants.viewProjection = m_ViewProjectionMatrix;
				pushConstants.normalMatrix = glm::transpose(glm::inverse(modelMatrix));
				pushConstants.cameraPosition = glm::vec4(m_CameraPosition, 1.0f);

				Buffer ConstantBuffer = Buffer::Copy(&pushConstants, sizeof(MeshPushConstants));

				Renderer::RenderStaticMeshWithMaterial(
					m_CommandBuffer, m_Pipeline, staticMesh, meshSource, i,
					m_TransformBuffer, 0, 1, m_Material, ConstantBuffer
				);

				ConstantBuffer.Release();
			}
		}
	}

	void MeshRenderer::TraverseNodeHierarchy(Ref<MeshSource> meshSource, Ref<StaticMesh> staticMesh,
		const std::vector<MeshNode>& nodes, uint32_t nodeIndex, const glm::mat4& parentTransform)
	{
		if (nodeIndex >= nodes.size())
			return;

		const auto& node = nodes[nodeIndex];
		glm::mat4 nodeTransform = parentTransform * node.LocalTransform;

		for (uint32_t submeshIndex : node.Submeshes) {
			const auto& submeshes = meshSource->GetSubmeshes();
			if (submeshIndex < submeshes.size()) {
				const auto& submesh = submeshes[submeshIndex];

				struct MeshPushConstants {
					alignas(16) glm::mat4 model;
					alignas(16) glm::mat4 viewProjection;
					alignas(16) glm::mat4 normalMatrix;
					alignas(16) glm::vec4 cameraPosition;
				};

				glm::mat4 modelMatrix = nodeTransform * submesh.Transform;
				MeshPushConstants pushConstants;
				pushConstants.model = modelMatrix;
				pushConstants.viewProjection = m_ViewProjectionMatrix;
				pushConstants.normalMatrix = glm::transpose(glm::inverse(modelMatrix));
				pushConstants.cameraPosition = glm::vec4(m_CameraPosition, 1.0f);

				Buffer ConstantBuffer = Buffer::Copy(&pushConstants, sizeof(MeshPushConstants));

				Renderer::RenderStaticMeshWithMaterial(
					m_CommandBuffer, m_Pipeline, staticMesh, meshSource, submeshIndex,
					m_TransformBuffer, 0, 1, m_Material, ConstantBuffer
				);

				ConstantBuffer.Release();
			}
		}

		// Recursively traverse children
		for (uint32_t childIndex : node.Children) {
			TraverseNodeHierarchy(meshSource, staticMesh, nodes, childIndex, nodeTransform);
		}
	}

	void MeshRenderer::EndScene()
	{
		if (!m_SceneActive)
			return;

		Renderer::EndRenderPass(m_CommandBuffer);

		m_CommandBuffer->End();
		m_CommandBuffer->Submit();

		m_SceneActive = false;
	}

	ImTextureID MeshRenderer::GetTextureImGuiID(Ref<Image2D> image)
	{
		if (!image)
			return (ImTextureID)nullptr;

		void* imagePtr = image.Raw();

		auto it = m_TextureDescriptorCache.find(imagePtr);
		if (it != m_TextureDescriptorCache.end())
		{
			return (ImTextureID)it->second;
		}

		auto vulkanImage = image.As<VulkanImage2D>();
		const VkDescriptorImageInfo& imageInfo = vulkanImage->GetDescriptorInfoVulkan();

		VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(
			imageInfo.sampler,
			imageInfo.imageView,
			imageInfo.imageLayout
		);

		m_TextureDescriptorCache[imagePtr] = descriptorSet;
		return (ImTextureID)descriptorSet;
	}

	void MeshRenderer::ClearTextureCache()
	{
		m_TextureDescriptorCache.clear();
	}

}