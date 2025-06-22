#pragma once

#include "Zenith/Core/Ref.hpp"
#include "Vulkan.hpp"
#include "VulkanDevice.hpp"
#include <vector>
#include <string>
#include <unordered_map>

namespace Zenith {
	enum class ShaderStage
	{
		Vertex,
		Fragment,
		Geometry,
		Compute
	};

	class VulkanShader : public RefCounted
	{
	public:
		VulkanShader(const std::string& filepath);
		VulkanShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);

		VulkanShader() = default;

		virtual ~VulkanShader();

		const std::string& GetName() const { return m_Name; }

		VkShaderModule GetVertexModule() const { return m_ShaderModules.at(ShaderStage::Vertex); }
		VkShaderModule GetFragmentModule() const { return m_ShaderModules.at(ShaderStage::Fragment); }

		bool HasStage(ShaderStage stage) const { return m_ShaderModules.find(stage) != m_ShaderModules.end(); }

		static Ref<VulkanShader> Create(const std::string& filepath);
		static Ref<VulkanShader> Create(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);

		static Ref<VulkanShader> CreateFromSpirv(const std::string& name,
												 const std::vector<uint32_t>& vertexSpirv,
												 const std::vector<uint32_t>& fragmentSpirv);

	private:
		void LoadFromFile(const std::string& filepath);
		void CompileShaders(const std::unordered_map<ShaderStage, std::string>& sources);
		void CreateShadersFromSpirv(const std::unordered_map<ShaderStage, std::vector<uint32_t>>& spirvCode);

		VkShaderModule CreateShaderModule(const std::vector<uint32_t>& code);

		std::string LoadTextFile(const std::string& filepath);

	private:
		std::string m_Name;
		std::unordered_map<ShaderStage, VkShaderModule> m_ShaderModules;
		Ref<VulkanDevice> m_Device;
	};
}