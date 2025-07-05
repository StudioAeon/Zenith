#include "znpch.hpp"
#include "Material.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanMaterial.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<Material> Material::Create(const Ref<Shader>& shader, const std::string& name)
	{
		return Ref<VulkanMaterial>::Create(shader, name);
	}
	
	Ref<Material> Material::Copy(const Ref<Material>& other, const std::string& name)
	{
		return Ref<VulkanMaterial>::Create(other, name);
	}

}