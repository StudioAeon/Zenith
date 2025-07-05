#include "znpch.hpp"
#include "Pipeline.hpp"

#include "Renderer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanPipeline.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<Pipeline> Pipeline::Create(const PipelineSpecification& spec)
	{
		return Ref<VulkanPipeline>::Create(spec);
	}

}