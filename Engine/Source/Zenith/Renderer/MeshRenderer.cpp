#include "znpch.hpp"
#include "MeshRenderer.hpp"

#include "Zenith/Core/Base.hpp"
#include "Zenith/Renderer/Renderer.hpp"
#include "Zenith/Asset/AssetManager.hpp"
#include "Zenith/Renderer/MaterialAsset.hpp"
#include "Zenith/Renderer/UniformBuffer.hpp"

namespace Zenith {

	MeshRenderer::MeshRenderer()
		: m_SceneActive(false)
	{
	}

	MeshRenderer::~MeshRenderer()
	{
		Shutdown();
	}

	void MeshRenderer::Initialize()
	{
		m_CommandBuffer = RenderCommandBuffer::Create();

		FramebufferSpecification framebufferSpec;
		framebufferSpec.DebugName = "MeshRenderer-Framebuffer";
		framebufferSpec.Width = 1280;
		framebufferSpec.Height = 720;
		framebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
		framebufferSpec.DepthClearValue = 1.0f;
		framebufferSpec.Attachments = {
			ImageFormat::RGBA32F,
			ImageFormat::DEPTH32F
		};
		framebufferSpec.SwapChainTarget = false;
		framebufferSpec.ClearColorOnLoad = true;
		framebufferSpec.ClearDepthOnLoad = true;

		m_Framebuffer = Framebuffer::Create(framebufferSpec);

		m_MeshShader = Renderer::GetShaderLibrary()->Get("PBR_StaticMesh");
		ZN_CORE_ASSERT(m_MeshShader, "Failed to load PBR_StaticMesh shader");

		m_MaterialUniformBuffer = UniformBuffer::Create(sizeof(MaterialUniforms));
		m_CameraUniformBuffer = UniformBuffer::Create(sizeof(CameraUniforms));

		CreatePipeline();
		CreateRenderPass();
	}

	void MeshRenderer::Shutdown()
	{
		ClearTextureCache();
		m_CachedStaticMeshes.clear();

		m_MaterialUniformBuffer = nullptr;
		m_CameraUniformBuffer = nullptr;
		m_CommandBuffer = nullptr;
		m_RenderPass = nullptr;
		m_Pipeline = nullptr;
		m_Framebuffer = nullptr;
		m_MeshShader = nullptr;
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
		pipelineSpec.DebugName = "MeshRenderer-PBR-Pipeline";
		pipelineSpec.Shader = m_MeshShader;
		pipelineSpec.TargetFramebuffer = m_Framebuffer;
		pipelineSpec.Layout = vertexLayout;
		pipelineSpec.BackfaceCulling = true;
		pipelineSpec.DepthTest = true;
		pipelineSpec.DepthWrite = true;
		pipelineSpec.Wireframe = false;
		pipelineSpec.DepthOperator = DepthCompareOperator::Less;
		pipelineSpec.Topology = PrimitiveTopology::Triangles;

		m_Pipeline = Pipeline::Create(pipelineSpec);
	}

	void MeshRenderer::CreateRenderPass()
	{
		RenderPassSpecification renderPassSpec;
		renderPassSpec.DebugName = "MeshRenderer-PBR-RenderPass";
		renderPassSpec.Pipeline = m_Pipeline;
		renderPassSpec.MarkerColor = { 0.2f, 0.8f, 0.2f, 1.0f };

		m_RenderPass = RenderPass::Create(renderPassSpec);
	}

	void MeshRenderer::BeginScene(const glm::mat4& viewProjection, const glm::vec3& cameraPosition)
	{
		m_ViewProjectionMatrix = viewProjection;
		m_CameraPosition = cameraPosition;
		m_SceneActive = true;

		CameraUniforms cameraData;
		cameraData.u_ViewProjection = viewProjection;
		cameraData.CameraPosition = cameraPosition;
		cameraData._Padding = 0.0f;

		m_CameraUniformBuffer->SetData(&cameraData, sizeof(CameraUniforms));

		m_CommandBuffer->Begin();
		Renderer::BeginRenderPass(m_CommandBuffer, m_RenderPass, true);
	}

	void MeshRenderer::DrawMesh(Ref<MeshSource> meshSource, const glm::mat4& transform)
	{
		if (!m_SceneActive || !meshSource)
			return;

		Ref<StaticMesh> staticMesh = GetOrCreateStaticMesh(meshSource);
		if (!staticMesh)
			return;

		const auto& nodes = meshSource->GetNodes();
		if (nodes.empty())
			return;

		for (uint32_t i = 0; i < nodes.size(); i++)
		{
			if (nodes[i].IsRoot())
			{
				TraverseNodeHierarchy(meshSource, staticMesh, nodes, i, transform);
			}
		}
	}

	void MeshRenderer::TraverseNodeHierarchy(Ref<MeshSource> meshSource, Ref<StaticMesh> staticMesh,
		const std::vector<MeshNode>& nodes, uint32_t nodeIndex, const glm::mat4& parentTransform)
	{
		const MeshNode& node = nodes[nodeIndex];
		glm::mat4 nodeTransform = parentTransform * node.LocalTransform;

		const auto& submeshes = meshSource->GetSubmeshes();
		for (uint32_t submeshIndex : node.Submeshes)
		{
			if (submeshIndex < submeshes.size())
			{
				RenderSubmesh(meshSource, staticMesh, submeshIndex, nodeTransform);
			}
		}

		for (uint32_t childIndex : node.Children)
		{
			TraverseNodeHierarchy(meshSource, staticMesh, nodes, childIndex, nodeTransform);
		}
	}

	void MeshRenderer::RenderSubmesh(Ref<MeshSource> meshSource, Ref<StaticMesh> staticMesh,
		uint32_t submeshIndex, const glm::mat4& transform)
	{
		const auto& submeshes = meshSource->GetSubmeshes();
		if (submeshIndex >= submeshes.size())
			return;

		const auto& submesh = submeshes[submeshIndex];

		Ref<MaterialAsset> materialAsset = GetMaterialForSubmesh(meshSource, submesh.MaterialIndex);
		if (!materialAsset)
		{
			ZN_CORE_WARN("No material found for submesh {}", submeshIndex);
			return;
		}

		Ref<Material> pbrMaterial = CreatePBRMaterial(materialAsset);
		if (!pbrMaterial)
		{
			ZN_CORE_ERROR("Failed to create PBR material for submesh {}", submeshIndex);
			return;
		}

		glm::mat4 finalTransform = transform * submesh.Transform;

		PBRPushConstants pushConstants;
		pushConstants.u_Transform = finalTransform;

		Buffer constantBuffer = Buffer::Copy(&pushConstants, sizeof(pushConstants));

		Renderer::RenderStaticMeshWithMaterial(
			m_CommandBuffer, m_Pipeline, staticMesh, meshSource, submeshIndex,
			nullptr, 0, 1, pbrMaterial, constantBuffer
		);

		constantBuffer.Release();
	}

	Ref<MaterialAsset> MeshRenderer::GetMaterialForSubmesh(Ref<MeshSource> meshSource, uint32_t materialIndex)
	{
		const auto& materials = meshSource->GetMaterials();
		if (materialIndex >= materials.size())
			return nullptr;

		AssetHandle materialHandle = materials[materialIndex];
		if (!materialHandle)
			return nullptr;

		return AssetManager::GetAsset<MaterialAsset>(materialHandle);
	}

	Ref<Material> MeshRenderer::CreatePBRMaterial(Ref<MaterialAsset> materialAsset)
	{
		const std::string shaderName = materialAsset->IsTransparent() ? "PBR_TransparentMesh" : "PBR_StaticMesh";
		Ref<Shader> shader = Renderer::GetShaderLibrary()->Get(shaderName);
		if (!shader)
		{
			ZN_CORE_ERROR("PBR shader '{}' not found", shaderName);
			return nullptr;
		}

		Ref<Material> material = Material::Create(shader, materialAsset->GetMaterial()->GetName());
		auto vulkanMaterial = material.As<VulkanMaterial>();
		if (!vulkanMaterial)
		{
			ZN_CORE_ERROR("Failed to cast material to VulkanMaterial");
			return nullptr;
		}

		MaterialUniforms materialData;
		materialData.u_AlbedoColor = materialAsset->GetAlbedoColor();
		materialData.u_Metalness = materialAsset->IsTransparent() ? 0.0f : materialAsset->GetMetalness();
		materialData.u_Roughness = materialAsset->IsTransparent() ? 1.0f : materialAsset->GetRoughness();
		materialData.u_Emission = materialAsset->GetEmission();
		materialData.u_UseNormalMap = materialAsset->IsUsingNormalMap() ? 1 : 0;
		materialData._Padding2 = 0.0f;

		Ref<UniformBuffer> materialUBO = UniformBuffer::Create(sizeof(MaterialUniforms));
		if (!materialUBO)
		{
			ZN_CORE_ERROR("Failed to create material uniform buffer");
			return nullptr;
		}

		materialUBO->SetData(&materialData, sizeof(MaterialUniforms));

		if (!m_CameraUniformBuffer)
		{
			ZN_CORE_ERROR("Camera uniform buffer is null");
			return nullptr;
		}

		vulkanMaterial->m_DescriptorSetManager.SetInput("MaterialUniformBuffer", materialUBO);
		vulkanMaterial->m_DescriptorSetManager.SetInput("CameraUniformBuffer", m_CameraUniformBuffer);

		if (!vulkanMaterial->m_DescriptorSetManager.HasInput("MaterialUniformBuffer") ||
			!vulkanMaterial->m_DescriptorSetManager.HasInput("CameraUniformBuffer"))
		{
			ZN_CORE_ERROR("Failed to set required uniform buffers for material: {}", materialAsset->GetMaterial()->GetName());
			return nullptr;
		}

		SetMaterialTextures(material, materialAsset);

		vulkanMaterial->m_DescriptorSetManager.Bake();
		return material;
	}

	void MeshRenderer::SetMaterialTextures(Ref<Material> material, Ref<MaterialAsset> materialAsset)
	{
		if (Ref<Texture2D> albedoTexture = materialAsset->GetAlbedoMap())
		{
			material->Set("u_AlbedoTexture", albedoTexture);
		}

		if (materialAsset->IsUsingNormalMap() && materialAsset->GetNormalMap())
		{
			material->Set("u_NormalTexture", materialAsset->GetNormalMap());
		}

		if (!materialAsset->IsTransparent())
		{
			if (Ref<Texture2D> metalnessTexture = materialAsset->GetMetalnessMap())
			{
				material->Set("u_MetalnessTexture", metalnessTexture);
			}

			if (Ref<Texture2D> roughnessTexture = materialAsset->GetRoughnessMap())
			{
				material->Set("u_RoughnessTexture", roughnessTexture);
			}
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