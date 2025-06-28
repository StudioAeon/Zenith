#include "znpch.hpp"
#include "MeshRenderer.hpp"

#include "Zenith/Renderer/Renderer.hpp"

#include "Zenith/Core/Application.hpp"
#include "Zenith/Asset/AssetManager.hpp"

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
		m_MeshShader = Shader::Create("Resources/Shaders/BasicMesh.hlsl");

		m_CommandBuffer = RenderCommandBuffer::Create();

		if (!m_Framebuffer)
		{
			FramebufferSpecification fbSpec;
			fbSpec.SwapChainTarget = false;
			fbSpec.Width = 1920;
			fbSpec.Height = 1080;
			fbSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
			fbSpec.DebugName = "MeshRenderer-SwapChain";

			fbSpec.Attachments = FramebufferAttachmentSpecification{
				{ ImageFormat::RGBA },
				{ ImageFormat::DEPTH24STENCIL8 }
			};

			m_Framebuffer = Framebuffer::Create(fbSpec);
		}

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
		//pipelineSpec.DepthOperator = DepthCompareOperator::Never;
		pipelineSpec.Topology = PrimitiveTopology::Triangles;

		m_Pipeline = Pipeline::Create(pipelineSpec);
	}

	void MeshRenderer::CreateRenderPass()
	{
		RenderPassSpecification renderPassSpec;
		renderPassSpec.DebugName = "MeshRenderer-RenderPass";
		renderPassSpec.Pipeline = m_Pipeline;
		renderPassSpec.MarkerColor = { 0.2f, 0.8f, 0.2f, 1.0f };

		m_RenderPass = RenderPass::Create(renderPassSpec);
	}

	void MeshRenderer::BeginScene(const glm::mat4& viewProjection)
	{
		m_ViewProjectionMatrix = viewProjection;
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

		glm::mat4 mvpMatrix = m_ViewProjectionMatrix * transform;

		Buffer ConstantBuffer = Buffer::Copy(&mvpMatrix, sizeof(glm::mat4));

		const auto& submeshes = meshSource->GetSubmeshes();
		for (uint32_t i = 0; i < submeshes.size(); i++)
		{
			Renderer::RenderStaticMeshWithMaterial(
				m_CommandBuffer,
				m_Pipeline,
				staticMesh,
				meshSource,
				i,
				m_TransformBuffer,
				0,
				1,
				m_MeshShader,
				ConstantBuffer
			);
		}

		ConstantBuffer.Release();
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