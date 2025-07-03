#include "znpch.hpp"
#include "DescriptorSetManager.hpp"

#include "Zenith/Renderer/Renderer.hpp"

#include "VulkanAPI.hpp"
#include "VulkanStorageBuffer.hpp"
#include "VulkanStorageBufferSet.hpp"
#include "VulkanUniformBuffer.hpp"
#include "VulkanUniformBufferSet.hpp"
#include "VulkanTexture.hpp"
#include "Zenith/Core/Timer.hpp"

#include "Zenith/Debug/Profiler.hpp"

namespace Zenith {
	
	namespace Utils {

		inline RenderPassResourceType GetDefaultResourceType(VkDescriptorType descriptorType)
		{
			switch (descriptorType)
			{
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
					return RenderPassResourceType::Texture2D;
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					return RenderPassResourceType::Image2D;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					return RenderPassResourceType::UniformBuffer;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
					return RenderPassResourceType::StorageBuffer;
				case VK_DESCRIPTOR_TYPE_SAMPLER:
					return RenderPassResourceType::Texture2D;
			}

			ZN_CORE_ASSERT(false);
			return RenderPassResourceType::None;
		}

	}

	DescriptorSetManager::DescriptorSetManager(const DescriptorSetManagerSpecification& specification)
		: m_Specification(specification)
	{
		Init();
	}

	DescriptorSetManager::DescriptorSetManager(const DescriptorSetManager& other)
		: m_Specification(other.m_Specification)
	{
		Init();
		InputResources = other.InputResources;
		Bake();
	}

	DescriptorSetManager DescriptorSetManager::Copy(const DescriptorSetManager& other)
	{
		DescriptorSetManager result(other);
		return result;
	}

	void DescriptorSetManager::Init()
	{
		const auto& shaderDescriptorSets = m_Specification.Shader->GetShaderDescriptorSets();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		WriteDescriptorMap.resize(framesInFlight);

		for (uint32_t set = m_Specification.StartSet; set <= m_Specification.EndSet; set++)
		{
			if (set >= shaderDescriptorSets.size())
				break;

			const auto& shaderDescriptor = shaderDescriptorSets[set];
			for (auto&& [bname, wd] : shaderDescriptor.WriteDescriptorSets)
			{
				const char* broken = strrchr(bname.c_str(), '.');
				std::string name = broken ? broken + 1 : bname;

				uint32_t binding = wd.dstBinding;
				RenderPassInputDeclaration& inputDecl = InputDeclarations[name];
				inputDecl.Type = RenderPassInputTypeFromVulkanDescriptorType(wd.descriptorType);
				inputDecl.Set = set;
				inputDecl.Binding = binding;
				inputDecl.Name = name;
				inputDecl.Count = wd.descriptorCount;

				RenderPassInput& input = InputResources[set][binding];
				input.Input.resize(wd.descriptorCount);
				input.Type = Utils::GetDefaultResourceType(wd.descriptorType);

				if (m_Specification.DefaultResources)
				{
					if (wd.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
						wd.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
					{
						if (inputDecl.Type == RenderPassInputType::ImageSampler2D)
						{
							for (size_t i = 0; i < input.Input.size(); i++)
								input.Input[i] = Renderer::GetWhiteTexture();
						}
						else if (inputDecl.Type == RenderPassInputType::ImageSampler3D)
						{
							for (size_t i = 0; i < input.Input.size(); i++)
								input.Input[i] = Renderer::GetBlackCubeTexture();
						}
					}
				}

				for (uint32_t frameIndex = 0; frameIndex < framesInFlight; frameIndex++)
					WriteDescriptorMap[frameIndex][set][binding] = { wd, std::vector<void*>(wd.descriptorCount) };

				if (shaderDescriptor.ImageSamplers.find(binding) != shaderDescriptor.ImageSamplers.end())
				{
					auto& imageSampler = shaderDescriptor.ImageSamplers.at(binding);
					uint32_t dimension = imageSampler.Dimension;
					if (wd.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || wd.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					{
						switch (dimension)
						{
						case 1:
							inputDecl.Type = RenderPassInputType::ImageSampler1D;
							break;
						case 2:
							inputDecl.Type = RenderPassInputType::ImageSampler2D;
							break;
						case 3:
							inputDecl.Type = RenderPassInputType::ImageSampler3D;
							break;
						}
					}
					else if (wd.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
					{
						switch (dimension)
						{
						case 1:
							inputDecl.Type = RenderPassInputType::StorageImage1D;
							break;
						case 2:
							inputDecl.Type = RenderPassInputType::StorageImage2D;
							break;
						case 3:
							inputDecl.Type = RenderPassInputType::StorageImage3D;
							break;
						}
					}
				}
			}
		}
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<UniformBufferSet> uniformBufferSet)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (decl)
		{
			// Ensure the set exists
			if (InputResources.find(decl->Set) == InputResources.end())
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Set {} not found for input {}",
					m_Specification.DebugName, decl->Set, name);
				return;
			}

			// Ensure the binding exists
			auto& setResources = InputResources[decl->Set];
			if (setResources.find(decl->Binding) == setResources.end())
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Binding {} not found in set {} for input {}",
					m_Specification.DebugName, decl->Binding, decl->Set, name);
				return;
			}

			setResources[decl->Binding].Set(uniformBufferSet);
		}
		else
			ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<UniformBuffer> uniformBuffer)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (decl)
		{
			// Ensure the set exists
			if (InputResources.find(decl->Set) == InputResources.end())
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Set {} not found for input {}",
					m_Specification.DebugName, decl->Set, name);
				return;
			}

			// Ensure the binding exists
			auto& setResources = InputResources[decl->Set];
			if (setResources.find(decl->Binding) == setResources.end())
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Binding {} not found in set {} for input {}",
					m_Specification.DebugName, decl->Binding, decl->Set, name);
				return;
			}

			setResources[decl->Binding].Set(uniformBuffer);
		}
		else
			ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<StorageBufferSet> storageBufferSet)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (decl)
		{
			// Ensure the set exists
			if (InputResources.find(decl->Set) == InputResources.end())
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Set {} not found for input {}",
					m_Specification.DebugName, decl->Set, name);
				return;
			}

			// Ensure the binding exists
			auto& setResources = InputResources[decl->Set];
			if (setResources.find(decl->Binding) == setResources.end())
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Binding {} not found in set {} for input {}",
					m_Specification.DebugName, decl->Binding, decl->Set, name);
				return;
			}

			setResources[decl->Binding].Set(storageBufferSet);
		}
		else
			ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<StorageBuffer> storageBuffer)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (decl)
		{
			// Ensure the set exists
			if (InputResources.find(decl->Set) == InputResources.end())
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Set {} not found for input {}",
					m_Specification.DebugName, decl->Set, name);
				return;
			}

			// Ensure the binding exists
			auto& setResources = InputResources[decl->Set];
			if (setResources.find(decl->Binding) == setResources.end())
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Binding {} not found in set {} for input {}",
					m_Specification.DebugName, decl->Binding, decl->Set, name);
				return;
			}

			setResources[decl->Binding].Set(storageBuffer);
		}
		else
			ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<Texture2D> texture, uint32_t index)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (decl)
		{
			if (index >= decl->Count)
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Index {} out of range for input {} (max: {})",
					m_Specification.DebugName, index, name, decl->Count - 1);
				return;
			}

			// Ensure the set exists
			if (InputResources.find(decl->Set) == InputResources.end())
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Set {} not found for input {}",
					m_Specification.DebugName, decl->Set, name);
				return;
			}

			// Ensure the binding exists
			auto& setResources = InputResources[decl->Set];
			if (setResources.find(decl->Binding) == setResources.end())
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Binding {} not found in set {} for input {}",
					m_Specification.DebugName, decl->Binding, decl->Set, name);
				return;
			}

			setResources[decl->Binding].Set(texture, index);
		}
		else
			ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<TextureCube> textureCube)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (decl)
		{
			// Ensure the set exists
			if (InputResources.find(decl->Set) == InputResources.end())
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Set {} not found for input {}",
					m_Specification.DebugName, decl->Set, name);
				return;
			}

			// Ensure the binding exists
			auto& setResources = InputResources[decl->Set];
			if (setResources.find(decl->Binding) == setResources.end())
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Binding {} not found in set {} for input {}",
					m_Specification.DebugName, decl->Binding, decl->Set, name);
				return;
			}

			setResources[decl->Binding].Set(textureCube);
		}
		else
			ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<Image2D> image)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (decl)
		{
			// Ensure the set exists
			if (InputResources.find(decl->Set) == InputResources.end())
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Set {} not found for input {}",
					m_Specification.DebugName, decl->Set, name);
				return;
			}

			// Ensure the binding exists
			auto& setResources = InputResources[decl->Set];
			if (setResources.find(decl->Binding) == setResources.end())
			{
				ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Binding {} not found in set {} for input {}",
					m_Specification.DebugName, decl->Binding, decl->Set, name);
				return;
			}

			setResources[decl->Binding].Set(image);
		}
		else
			ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<ImageView> image)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (!decl)
		{
			ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
			return;
		}

		// Ensure the set exists
		if (InputResources.find(decl->Set) == InputResources.end())
		{
			ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Set {} not found for input {}",
				m_Specification.DebugName, decl->Set, name);
			return;
		}

		// Ensure the binding exists
		auto& setResources = InputResources[decl->Set];
		if (setResources.find(decl->Binding) == setResources.end())
		{
			ZN_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Binding {} not found in set {} for input {}",
				m_Specification.DebugName, decl->Binding, decl->Set, name);
			return;
		}

		setResources[decl->Binding].Set(image);
	}

	bool DescriptorSetManager::IsInvalidated(uint32_t set, uint32_t binding) const
	{
		if (InvalidatedInputResources.find(set) != InvalidatedInputResources.end())
		{
			const auto& resources = InvalidatedInputResources.at(set);
			return resources.find(binding) != resources.end();
		}
		return false;
	}

	std::set<uint32_t> DescriptorSetManager::HasBufferSets() const
	{
		std::set<uint32_t> sets;
		for (const auto& [set, resources] : InputResources)
		{
			for (const auto& [binding, input] : resources)
			{
				if (input.Type == RenderPassResourceType::UniformBufferSet || input.Type == RenderPassResourceType::StorageBufferSet)
				{
					sets.insert(set);
					break;
				}
			}
		}
		return sets;
	}


	bool DescriptorSetManager::Validate()
	{
		const auto& shaderDescriptorSets = m_Specification.Shader->GetShaderDescriptorSets();
		bool allValid = true;

		for (uint32_t set = m_Specification.StartSet; set <= m_Specification.EndSet; set++)
		{
			if (set >= shaderDescriptorSets.size())
				break;

			if (!shaderDescriptorSets[set])
				continue;

			if (InputResources.find(set) == InputResources.end())
			{
				ZN_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] No input resources for Set {}", m_Specification.DebugName, set);
				allValid = false;
				continue;
			}

			const auto& setInputResources = InputResources.at(set);
			const auto& shaderDescriptor = shaderDescriptorSets[set];

			for (auto&& [name, wd] : shaderDescriptor.WriteDescriptorSets)
			{
				uint32_t binding = wd.dstBinding;
				if (setInputResources.find(binding) == setInputResources.end())
				{
					ZN_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] No input resource for {}.{}", m_Specification.DebugName, set, binding);
					ZN_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] Required resource is {} ({})", m_Specification.DebugName, name, (int)wd.descriptorType);
					allValid = false;
					continue;
				}

				const auto& resource = setInputResources.at(binding);

				if (!IsCompatibleInput(resource.Type, wd.descriptorType))
				{
					ZN_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] Type mismatch for '{}': Got {} but shader expects {}",
						m_Specification.DebugName, name,
						static_cast<int>(resource.Type), static_cast<int>(wd.descriptorType));
					allValid = false;
					continue;
				}

				if (resource.Type != RenderPassResourceType::Image2D &&
					(resource.Input.empty() || resource.Input[0] == nullptr))
				{
					ZN_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] Resource '{}' is null! ({}.{})",
						m_Specification.DebugName, name, set, binding);
					allValid = false;
				}
			}
		}

		if (!allValid)
			ZN_CORE_ERROR("Validation failed for {}", m_Specification.DebugName);

		return allValid;
	}

	void DescriptorSetManager::Bake()
	{
		if (!Validate())
		{
			ZN_CORE_ERROR_TAG("Renderer", "[RenderPass] Bake - Validate failed! {}", m_Specification.DebugName);
			return;
		}

		// If valid, we can create descriptor sets

		// Create Descriptor Pool
		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 10 * 3; // frames in flight should partially determine this
		poolInfo.poolSizeCount = 10;
		poolInfo.pPoolSizes = poolSizes;

		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool));

		auto bufferSets = HasBufferSets();
		bool perFrameInFlight = !bufferSets.empty();
		perFrameInFlight = true; // always
		uint32_t descriptorSetCount = Renderer::GetConfig().FramesInFlight;
		if (!perFrameInFlight)
			descriptorSetCount = 1;

		if (m_DescriptorSets.size() < 1)
		{
			for (uint32_t i = 0; i < descriptorSetCount; i++)
				m_DescriptorSets.emplace_back();
		}

		for (auto& descriptorSet : m_DescriptorSets)
			descriptorSet.clear();

		for (const auto& [set, setData] : InputResources)
		{
			uint32_t descriptorCountInSet = bufferSets.find(set) != bufferSets.end() ? descriptorSetCount : 1;
			for (uint32_t frameIndex = 0; frameIndex < descriptorSetCount; frameIndex++)
			{
				VkDescriptorSetLayout dsl = m_Specification.Shader->GetDescriptorSetLayout(set);
				VkDescriptorSetAllocateInfo descriptorSetAllocInfo = Vulkan::DescriptorSetAllocInfo(&dsl);
				descriptorSetAllocInfo.descriptorPool = m_DescriptorPool;
				VkDescriptorSet descriptorSet = nullptr;
				VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet));

				m_DescriptorSets[frameIndex].emplace_back(descriptorSet);

				auto& writeDescriptorMap = WriteDescriptorMap[frameIndex].at(set);
				std::vector<std::vector<VkDescriptorImageInfo>> imageInfoStorage;
				uint32_t imageInfoStorageIndex = 0;

				for (const auto& [binding, input] : setData)
				{
					// Safely check if the binding exists in the write descriptor map
					auto writeDescriptorMapIt = WriteDescriptorMap[frameIndex].find(set);
					if (writeDescriptorMapIt == WriteDescriptorMap[frameIndex].end())
					{
						ZN_CORE_ERROR_TAG("Renderer", "[RenderPass] Bake - Set {} not found in WriteDescriptorMap", set);
						continue;
					}

					auto& writeDescriptorMap = writeDescriptorMapIt->second;
					auto storedWriteDescriptorIt = writeDescriptorMap.find(binding);
					if (storedWriteDescriptorIt == writeDescriptorMap.end())
					{
						ZN_CORE_ERROR_TAG("Renderer", "[RenderPass] Bake - Binding {} not found in WriteDescriptorMap", binding);
						continue;
					}

					auto& storedWriteDescriptor = storedWriteDescriptorIt->second;

					VkWriteDescriptorSet& writeDescriptor = storedWriteDescriptor.WriteDescriptorSet;
					writeDescriptor.dstSet = descriptorSet;

					// ... rest of the switch statement remains the same ...
					switch (input.Type)
					{
					case RenderPassResourceType::UniformBuffer:
					{
						Ref<VulkanUniformBuffer> buffer = input.Input[0].As<VulkanUniformBuffer>();
						writeDescriptor.pBufferInfo = &buffer->GetDescriptorBufferInfo();
						storedWriteDescriptor.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;

						// Defer if resource doesn't exist
						if (writeDescriptor.pBufferInfo->buffer == nullptr)
							InvalidatedInputResources[set][binding] = input;

						break;
					}
					case RenderPassResourceType::UniformBufferSet:
					{
						Ref<UniformBufferSet> buffer = input.Input[0].As<UniformBufferSet>();
						// TODO: replace 0 with current frame in flight (i.e. create bindings for all frames)
						writeDescriptor.pBufferInfo = &buffer->Get(frameIndex).As<VulkanUniformBuffer>()->GetDescriptorBufferInfo();
						storedWriteDescriptor.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;

						// Defer if resource doesn't exist
						if (writeDescriptor.pBufferInfo->buffer == nullptr)
							InvalidatedInputResources[set][binding] = input;

						break;
					}
					case RenderPassResourceType::StorageBuffer:
					{
						Ref<VulkanStorageBuffer> buffer = input.Input[0].As<VulkanStorageBuffer>();
						writeDescriptor.pBufferInfo = &buffer->GetDescriptorBufferInfo();
						storedWriteDescriptor.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;

						// Defer if resource doesn't exist
						if (writeDescriptor.pBufferInfo->buffer == nullptr)
							InvalidatedInputResources[set][binding] = input;

						break;
					}
					case RenderPassResourceType::StorageBufferSet:
					{
						Ref<StorageBufferSet> buffer = input.Input[0].As<StorageBufferSet>();
						// TODO: replace 0 with current frame in flight (i.e. create bindings for all frames)
						writeDescriptor.pBufferInfo = &buffer->Get(frameIndex).As<VulkanStorageBuffer>()->GetDescriptorBufferInfo();
						storedWriteDescriptor.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;

						// Defer if resource doesn't exist
						if (writeDescriptor.pBufferInfo->buffer == nullptr)
							InvalidatedInputResources[set][binding] = input;

						break;
					}
					case RenderPassResourceType::Texture2D:
					{
						if (input.Input.size() > 1)
						{
							imageInfoStorage.emplace_back(input.Input.size());
							for (size_t i = 0; i < input.Input.size(); i++)
							{
								Ref<VulkanTexture2D> texture = input.Input[i].As<VulkanTexture2D>();
								imageInfoStorage[imageInfoStorageIndex][i] = texture->GetDescriptorInfoVulkan();

							}
							writeDescriptor.pImageInfo = imageInfoStorage[imageInfoStorageIndex].data();
							imageInfoStorageIndex++;
						}
						else
						{
							Ref<VulkanTexture2D> texture = input.Input[0].As<VulkanTexture2D>();
							writeDescriptor.pImageInfo = &texture->GetDescriptorInfoVulkan();
						}
						storedWriteDescriptor.ResourceHandles[0] = writeDescriptor.pImageInfo->imageView;

						// Defer if resource doesn't exist
						if (writeDescriptor.pImageInfo->imageView == nullptr)
							InvalidatedInputResources[set][binding] = input;

						break;
					}
					case RenderPassResourceType::TextureCube:
					{
						Ref<VulkanTextureCube> texture = input.Input[0].As<VulkanTextureCube>();
						writeDescriptor.pImageInfo = &texture->GetDescriptorInfoVulkan();
						storedWriteDescriptor.ResourceHandles[0] = writeDescriptor.pImageInfo->imageView;

						// Defer if resource doesn't exist
						if (writeDescriptor.pImageInfo->imageView == nullptr)
							InvalidatedInputResources[set][binding] = input;

						break;
					}
					case RenderPassResourceType::Image2D:
					{
						Ref<RendererResource> image = input.Input[0].As<RendererResource>();
						// Defer if resource doesn't exist
						if (image == nullptr)
						{
							InvalidatedInputResources[set][binding] = input;
							break;
						}

						writeDescriptor.pImageInfo = (VkDescriptorImageInfo*)image->GetDescriptorInfo();
						storedWriteDescriptor.ResourceHandles[0] = writeDescriptor.pImageInfo->imageView;

						// Defer if resource doesn't exist
						if (writeDescriptor.pImageInfo->imageView == nullptr)
							InvalidatedInputResources[set][binding] = input;

						break;
					}
					}
				}

				std::vector<VkWriteDescriptorSet> writeDescriptors;
				auto writeDescriptorMapIt = WriteDescriptorMap[frameIndex].find(set);
				if (writeDescriptorMapIt != WriteDescriptorMap[frameIndex].end())
				{
					for (auto&& [binding, writeDescriptor] : writeDescriptorMapIt->second)
					{
						// Include if valid, otherwise defer (these will be resolved if possible at Prepare stage)
						if (!IsInvalidated(set, binding))
							writeDescriptors.emplace_back(writeDescriptor.WriteDescriptorSet);
					}
				}

				if (!writeDescriptors.empty())
				{
					vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
				}
			}
		}

	}

	void DescriptorSetManager::InvalidateAndUpdate()
	{
		ZN_PROFILE_FUNC();
		ZN_SCOPE_PERF("DescriptorSetManager::InvalidateAndUpdate");

		uint32_t currentFrameIndex = Renderer::RT_GetCurrentFrameIndex();

		for (const auto& [set, inputs] : InputResources)
		{
			for (const auto& [binding, input] : inputs)
			{
				if (currentFrameIndex >= WriteDescriptorMap.size())
					continue;

				auto frameMapIt = WriteDescriptorMap[currentFrameIndex].find(set);
				if (frameMapIt == WriteDescriptorMap[currentFrameIndex].end())
					continue;

				auto bindingMapIt = frameMapIt->second.find(binding);
				if (bindingMapIt == frameMapIt->second.end())
					continue;

				switch (input.Type)
				{
				case RenderPassResourceType::UniformBuffer:
				{
					const VkDescriptorBufferInfo& bufferInfo = input.Input[0].As<VulkanUniformBuffer>()->GetDescriptorBufferInfo();
					if (bufferInfo.buffer != bindingMapIt->second.ResourceHandles[0])
						InvalidatedInputResources[set][binding] = input;
					break;
				}
				case RenderPassResourceType::UniformBufferSet:
				{
					const VkDescriptorBufferInfo& bufferInfo = input.Input[0].As<VulkanUniformBufferSet>()->Get(currentFrameIndex).As<VulkanUniformBuffer>()->GetDescriptorBufferInfo();
					if (bufferInfo.buffer != bindingMapIt->second.ResourceHandles[0])
						InvalidatedInputResources[set][binding] = input;
					break;
				}
				case RenderPassResourceType::StorageBuffer:
				{
					const VkDescriptorBufferInfo& bufferInfo = input.Input[0].As<VulkanStorageBuffer>()->GetDescriptorBufferInfo();
					if (bufferInfo.buffer != bindingMapIt->second.ResourceHandles[0])
						InvalidatedInputResources[set][binding] = input;
					break;
				}
				case RenderPassResourceType::StorageBufferSet:
				{
					const VkDescriptorBufferInfo& bufferInfo = input.Input[0].As<VulkanStorageBufferSet>()->Get(currentFrameIndex).As<VulkanStorageBuffer>()->GetDescriptorBufferInfo();
					if (bufferInfo.buffer != bindingMapIt->second.ResourceHandles[0])
						InvalidatedInputResources[set][binding] = input;
					break;
				}
				case RenderPassResourceType::Texture2D:
				{
					for (size_t i = 0; i < input.Input.size(); i++)
					{
						Ref<VulkanTexture2D> vulkanTexture = input.Input[i].As<VulkanTexture2D>();
						if (!vulkanTexture)
							vulkanTexture = Renderer::GetWhiteTexture().As<VulkanTexture2D>();

						const VkDescriptorImageInfo& imageInfo = vulkanTexture->GetDescriptorInfoVulkan();
						if (i < bindingMapIt->second.ResourceHandles.size() &&
							imageInfo.imageView != bindingMapIt->second.ResourceHandles[i])
						{
							InvalidatedInputResources[set][binding] = input;
							break;
						}
					}
					break;
				}
				case RenderPassResourceType::TextureCube:
				{
					const VkDescriptorImageInfo& imageInfo = input.Input[0].As<VulkanTextureCube>()->GetDescriptorInfoVulkan();
					if (imageInfo.imageView != bindingMapIt->second.ResourceHandles[0])
						InvalidatedInputResources[set][binding] = input;
					break;
				}
				case RenderPassResourceType::Image2D:
				{
					const VkDescriptorImageInfo& imageInfo = *(VkDescriptorImageInfo*)input.Input[0].As<RendererResource>()->GetDescriptorInfo();
					if (imageInfo.imageView != bindingMapIt->second.ResourceHandles[0])
						InvalidatedInputResources[set][binding] = input;
					break;
				}
				}
			}
		}

		if (InvalidatedInputResources.empty())
			return;

		auto bufferSets = HasBufferSets();
		bool perFrameInFlight = !bufferSets.empty();
		perFrameInFlight = true; // always true anyway
		uint32_t descriptorSetCount = Renderer::GetConfig().FramesInFlight;
		if (!perFrameInFlight)
			descriptorSetCount = 1;

		for (const auto& [set, setData] : InvalidatedInputResources)
		{
			uint32_t frameIndex = perFrameInFlight ? currentFrameIndex : 0;

			if (frameIndex >= WriteDescriptorMap.size())
				continue;

			auto frameMapIt = WriteDescriptorMap[frameIndex].find(set);
			if (frameMapIt == WriteDescriptorMap[frameIndex].end())
				continue;

			std::vector<VkWriteDescriptorSet> writeDescriptorsToUpdate;
			std::vector<std::vector<VkDescriptorImageInfo>> imageInfoStorage;
			uint32_t imageInfoStorageIndex = 0;

			for (const auto& [binding, input] : setData)
			{
				auto bindingMapIt = frameMapIt->second.find(binding);
				if (bindingMapIt == frameMapIt->second.end())
					continue;

				auto& wd = bindingMapIt->second;
				VkWriteDescriptorSet& writeDescriptor = wd.WriteDescriptorSet;

				switch (input.Type)
				{
				case RenderPassResourceType::UniformBuffer:
				{
					Ref<VulkanUniformBuffer> buffer = input.Input[0].As<VulkanUniformBuffer>();
					writeDescriptor.pBufferInfo = &buffer->GetDescriptorBufferInfo();
					wd.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;
					break;
				}
				case RenderPassResourceType::UniformBufferSet:
				{
					Ref<UniformBufferSet> buffer = input.Input[0].As<UniformBufferSet>();
					writeDescriptor.pBufferInfo = &buffer->Get(frameIndex).As<VulkanUniformBuffer>()->GetDescriptorBufferInfo();
					wd.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;
					break;
				}
				case RenderPassResourceType::StorageBuffer:
				{
					Ref<VulkanStorageBuffer> buffer = input.Input[0].As<VulkanStorageBuffer>();
					writeDescriptor.pBufferInfo = &buffer->GetDescriptorBufferInfo();
					wd.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;
					break;
				}
				case RenderPassResourceType::StorageBufferSet:
				{
					Ref<StorageBufferSet> buffer = input.Input[0].As<StorageBufferSet>();
					writeDescriptor.pBufferInfo = &buffer->Get(frameIndex).As<VulkanStorageBuffer>()->GetDescriptorBufferInfo();
					wd.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;
					break;
				}
				case RenderPassResourceType::Texture2D:
				{
					if (input.Input.size() > 1)
					{
						imageInfoStorage.emplace_back(input.Input.size());
						for (size_t i = 0; i < input.Input.size(); i++)
						{
							Ref<VulkanTexture2D> texture = input.Input[i].As<VulkanTexture2D>();
							imageInfoStorage[imageInfoStorageIndex][i] = texture->GetDescriptorInfoVulkan();
							if (i < wd.ResourceHandles.size())
								wd.ResourceHandles[i] = imageInfoStorage[imageInfoStorageIndex][i].imageView;
						}
						writeDescriptor.pImageInfo = imageInfoStorage[imageInfoStorageIndex].data();
						imageInfoStorageIndex++;
					}
					else
					{
						Ref<VulkanTexture2D> texture = input.Input[0].As<VulkanTexture2D>();
						writeDescriptor.pImageInfo = &texture->GetDescriptorInfoVulkan();
						wd.ResourceHandles[0] = writeDescriptor.pImageInfo->imageView;
					}
					break;
				}
				case RenderPassResourceType::TextureCube:
				{
					Ref<VulkanTextureCube> texture = input.Input[0].As<VulkanTextureCube>();
					writeDescriptor.pImageInfo = &texture->GetDescriptorInfoVulkan();
					wd.ResourceHandles[0] = writeDescriptor.pImageInfo->imageView;
					break;
				}
				case RenderPassResourceType::Image2D:
				{
					Ref<RendererResource> image = input.Input[0].As<RendererResource>();
					writeDescriptor.pImageInfo = (VkDescriptorImageInfo*)image->GetDescriptorInfo();
					ZN_CORE_VERIFY(writeDescriptor.pImageInfo->imageView);
					wd.ResourceHandles[0] = writeDescriptor.pImageInfo->imageView;
					break;
				}
				}

				writeDescriptorsToUpdate.emplace_back(writeDescriptor);
			}

			if (!writeDescriptorsToUpdate.empty())
			{
				VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
				vkUpdateDescriptorSets(device, (uint32_t)writeDescriptorsToUpdate.size(), writeDescriptorsToUpdate.data(), 0, nullptr);
			}
		}

		InvalidatedInputResources.clear();
	}

	bool DescriptorSetManager::HasDescriptorSets() const
	{
		return !m_DescriptorSets.empty() && !m_DescriptorSets[0].empty();
	}

	uint32_t DescriptorSetManager::GetFirstSetIndex() const
	{
		if (InputResources.empty())
			return UINT32_MAX;

		// Return first key (key == descriptor set index)
		return InputResources.begin()->first;
	}

	const std::vector<VkDescriptorSet>& DescriptorSetManager::GetDescriptorSets(uint32_t frameIndex) const
	{
		ZN_CORE_ASSERT(!m_DescriptorSets.empty());

		if (frameIndex > 0 && m_DescriptorSets.size() == 1)
			return m_DescriptorSets[0]; // Frame index is irrelevant for this type of render pass

		return m_DescriptorSets[frameIndex];
	}

	bool DescriptorSetManager::IsInputValid(std::string_view name) const
	{
		std::string nameStr(name);
		return InputDeclarations.find(nameStr) != InputDeclarations.end();
	}

	const RenderPassInputDeclaration* DescriptorSetManager::GetInputDeclaration(std::string_view name) const
	{
		std::string nameStr(name);
		if (InputDeclarations.find(nameStr) == InputDeclarations.end())
			return nullptr;

		const RenderPassInputDeclaration& decl = InputDeclarations.at(nameStr);
		return &decl;
	}

}
