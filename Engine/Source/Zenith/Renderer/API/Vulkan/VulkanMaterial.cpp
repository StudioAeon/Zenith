#include "znpch.hpp"
#include "VulkanMaterial.hpp"

#include "VulkanAPI.hpp"
#include "VulkanContext.hpp"
#include "VulkanImage.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanRenderer.hpp"
#include "VulkanTexture.hpp"
#include "VulkanUniformBuffer.hpp"

#include "Zenith/Core/Timer.hpp"
#include "Zenith/Renderer/Renderer.hpp"

#include <format>

namespace Zenith {

	VulkanMaterial::VulkanMaterial(const Ref<Shader>& shader, const std::string& name)
		: m_Shader(shader.As<VulkanShader>()), m_Name(name)
	{
		Init();
		Renderer::RegisterShaderDependency(shader, this);
	}

	VulkanMaterial::VulkanMaterial(Ref<Material> material, const std::string& name)
		: m_Shader(material->GetShader().As<VulkanShader>()), m_Name(name)
	{
		if (name.empty())
			m_Name = material->GetName();

		Init();
		Renderer::RegisterShaderDependency(m_Shader, this);

		auto vulkanMaterial = material.As<VulkanMaterial>();
		m_UniformStorageBuffer = Buffer::Copy(vulkanMaterial->m_UniformStorageBuffer.Data, vulkanMaterial->m_UniformStorageBuffer.Size);
		m_DescriptorSetManager = DescriptorSetManager::Copy(vulkanMaterial->m_DescriptorSetManager);
	}

	VulkanMaterial::~VulkanMaterial()
	{
		m_UniformStorageBuffer.Release();
	}

	void VulkanMaterial::Init()
	{
		AllocateStorage();

		m_MaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
		m_MaterialFlags |= (uint32_t)MaterialFlag::Blend;

		DescriptorSetManagerSpecification dmSpec;
		dmSpec.DebugName = m_Name.empty() ? std::format("{} (Material)", m_Shader->GetName()) : m_Name;
		dmSpec.Shader = m_Shader.As<VulkanShader>();
		dmSpec.StartSet = 0;
		dmSpec.EndSet = 0;
		dmSpec.DefaultResources = true;
		m_DescriptorSetManager = DescriptorSetManager(dmSpec);

		for (const auto& [name, decl] : m_DescriptorSetManager.InputDeclarations)
		{
			switch (decl.Type)
			{
				case RenderPassInputType::UniformBuffer:
				case RenderPassInputType::StorageBuffer:
				case RenderPassInputType::ImageSampler1D:
				case RenderPassInputType::ImageSampler2D:
				case RenderPassInputType::ImageSampler3D:
				case RenderPassInputType::StorageImage1D:
				case RenderPassInputType::StorageImage2D:
				case RenderPassInputType::StorageImage3D:
					break;
				default:
					break;
			}
		}
	}

	void VulkanMaterial::Invalidate()
	{
		if (m_Shader->HasDescriptorSet(0))
		{
			VkDescriptorSetLayout dsl = m_Shader->GetDescriptorSetLayout(0);
			VkDescriptorSetAllocateInfo descriptorSetAllocInfo = Vulkan::DescriptorSetAllocInfo(&dsl);
			uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
			m_MaterialDescriptorSets.resize(framesInFlight);
			for (uint32_t i = 0; i < framesInFlight; i++)
				m_MaterialDescriptorSets[i] = VulkanRenderer::AllocateMaterialDescriptorSet(descriptorSetAllocInfo);

			const auto& shaderDescriptorSets = m_Shader->GetShaderDescriptorSets();
			std::map<uint32_t, VkWriteDescriptorSet> writeDescriptors;

			for (const auto& [name, writeDescriptor] : shaderDescriptorSets[0].WriteDescriptorSets)
			{
				if (writeDescriptor.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
					writeDescriptor.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
					writeDescriptor.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
				{
					writeDescriptors[writeDescriptor.dstBinding] = writeDescriptor;
				}
			}

			for (const auto& [binding, writeDescriptor] : writeDescriptors)
			{
				m_MaterialWriteDescriptors[binding] = writeDescriptor;
				m_MaterialDescriptorImages[binding] = std::vector<Ref<RendererResource>>(writeDescriptor.descriptorCount);

				if (writeDescriptor.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				{
					for (size_t i = 0; i < writeDescriptor.descriptorCount; i++)
						m_MaterialDescriptorImages[binding][i] = Renderer::GetWhiteTexture();
				}
			}
		}
		else
		{
			ZN_CORE_WARN_TAG("Renderer", "[Material] - shader {} has no Set 0!", m_Shader->GetName());
		}
	}

	void VulkanMaterial::AllocateStorage()
	{
		const auto& shaderBuffers = m_Shader->GetShaderBuffers();

		if (shaderBuffers.size() > 0)
		{
			uint32_t size = 0;
			for (auto [name, shaderBuffer] : shaderBuffers)
				size += shaderBuffer.Size;

			m_UniformStorageBuffer.Allocate(size);
			m_UniformStorageBuffer.ZeroInitialize();
		}
	}

	void VulkanMaterial::OnShaderReloaded()
	{
		//Init();
	}

	const ShaderUniform* VulkanMaterial::FindUniformDeclaration(const std::string& name)
	{
		const auto& shaderBuffers = m_Shader->GetShaderBuffers();

		if (shaderBuffers.contains("MaterialUniformBuffer"))
		{
			const ShaderBuffer& materialBuffer = shaderBuffers.at("MaterialUniformBuffer");
			if (materialBuffer.Uniforms.find(name) != materialBuffer.Uniforms.end())
			{
				return &materialBuffer.Uniforms.at(name);
			}
		}

		for (const auto& [bufferName, buffer] : shaderBuffers)
		{
			if (bufferName == "MaterialUniformBuffer")
				continue;

			if (buffer.Uniforms.find(name) != buffer.Uniforms.end())
			{
				return &buffer.Uniforms.at(name);
			}
		}

		return nullptr;
	}

	const ShaderResourceDeclaration* VulkanMaterial::FindResourceDeclaration(const std::string& name)
	{
		auto& resources = m_Shader->GetResources();
		if (resources.find(name) != resources.end())
			return &resources.at(name);

		return nullptr;
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture)
	{
		m_DescriptorSetManager.SetInput(name, texture);
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex)
	{
		m_DescriptorSetManager.SetInput(name, texture, arrayIndex);
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<TextureCube>& texture)
	{
		m_DescriptorSetManager.SetInput(name, texture);
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Image2D>& image)
	{
		ZN_CORE_VERIFY(image);
		m_DescriptorSetManager.SetInput(name, image);
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<ImageView>& image)
	{
		ZN_CORE_VERIFY(image);
		m_DescriptorSetManager.SetInput(name, image);
	}

	void VulkanMaterial::Set(const std::string& name, float value)
	{
		Set<float>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, int value)
	{
		Set<int>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, uint32_t value)
	{
		Set<uint32_t>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, bool value)
	{
		// Bools are 4-byte ints
		Set<int>(name, (int)value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::ivec2& value)
	{
		Set<glm::ivec2>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::ivec3& value)
	{
		Set<glm::ivec3>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::ivec4& value)
	{
		Set<glm::ivec4>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::vec2& value)
	{
		Set<glm::vec2>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::vec3& value)
	{
		Set<glm::vec3>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::vec4& value)
	{
		Set<glm::vec4>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::mat3& value)
	{
		Set<glm::mat3>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::mat4& value)
	{
		Set<glm::mat4>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const Ref<Texture2D>& texture)
	{
		SetVulkanDescriptor(name, texture);
	}

	void VulkanMaterial::Set(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex)
	{
		SetVulkanDescriptor(name, texture, arrayIndex);
	}

	void VulkanMaterial::Set(const std::string& name, const Ref<TextureCube>& texture)
	{
		SetVulkanDescriptor(name, texture);
	}

	void VulkanMaterial::Set(const std::string& name, const Ref<Image2D>& image)
	{
		SetVulkanDescriptor(name, image);
	}

	void VulkanMaterial::Set(const std::string& name, const Ref<ImageView>& image)
	{
		SetVulkanDescriptor(name, image);
	}

	float& VulkanMaterial::GetFloat(const std::string& name)
	{
		return Get<float>(name);
	}

	int32_t& VulkanMaterial::GetInt(const std::string& name)
	{
		return Get<int32_t>(name);
	}

	uint32_t& VulkanMaterial::GetUInt(const std::string& name)
	{
		return Get<uint32_t>(name);
	}

	bool& VulkanMaterial::GetBool(const std::string& name)
	{
		return Get<bool>(name);
	}

	glm::vec2& VulkanMaterial::GetVector2(const std::string& name)
	{
		return Get<glm::vec2>(name);
	}

	glm::vec3& VulkanMaterial::GetVector3(const std::string& name)
	{
		return Get<glm::vec3>(name);
	}

	glm::vec4& VulkanMaterial::GetVector4(const std::string& name)
	{
		return Get<glm::vec4>(name);
	}

	glm::mat3& VulkanMaterial::GetMatrix3(const std::string& name)
	{
		return Get<glm::mat3>(name);
	}

	glm::mat4& VulkanMaterial::GetMatrix4(const std::string& name)
	{
		return Get<glm::mat4>(name);
	}

	Ref<Texture2D> VulkanMaterial::GetTexture2D(const std::string& name)
	{
		return GetResource<Texture2D>(name);
	}

	Ref<TextureCube> VulkanMaterial::TryGetTextureCube(const std::string& name)
	{
		return TryGetResource<TextureCube>(name);
	}

	Ref<Texture2D> VulkanMaterial::TryGetTexture2D(const std::string& name)
	{
		return TryGetResource<Texture2D>(name);
	}

	Ref<TextureCube> VulkanMaterial::GetTextureCube(const std::string& name)
	{
		return GetResource<TextureCube>(name);
	}

	void VulkanMaterial::Prepare()
	{
		m_DescriptorSetManager.InvalidateAndUpdate();
	}

}