#include "znpch.hpp"
#include "UniformBuffer.hpp"

#include "Zenith/Renderer/Renderer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanUniformBuffer.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size)
	{
		return Ref<VulkanUniformBuffer>::Create(size);
	}

}
