#include "znpch.hpp"
#include "Framebuffer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanFramebuffer.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		Ref<Framebuffer> result = nullptr;

		result = Ref<VulkanFramebuffer>::Create(spec);
		return result;
	}

}
