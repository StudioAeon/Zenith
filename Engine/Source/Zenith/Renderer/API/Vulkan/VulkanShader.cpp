#include "znpch.hpp"
#include "VulkanShader.hpp"
#include "VulkanContext.hpp"
#include <fstream>
#include <filesystem>

#include "DXCCompiler.hpp"

namespace Zenith {

	VulkanShader::VulkanShader(const std::string& filepath)
		: m_Device(VulkanContext::GetCurrentDevice())
	{
		LoadFromFile(filepath);
	}

	VulkanShader::VulkanShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
		: m_Name(name), m_Device(VulkanContext::GetCurrentDevice())
	{
		std::unordered_map<ShaderStage, std::string> sources;
		sources[ShaderStage::Vertex] = vertexSource;
		sources[ShaderStage::Fragment] = fragmentSource;
		CompileShaders(sources);
	}

	VulkanShader::~VulkanShader()
	{
		for (auto& [stage, module] : m_ShaderModules)
		{
			vkDestroyShaderModule(m_Device->GetVulkanDevice(), module, nullptr);
		}
	}

	void VulkanShader::LoadFromFile(const std::string& filepath)
	{
		std::filesystem::path path(filepath);
		m_Name = path.stem().string();

		std::string hlslSource = LoadTextFile(filepath);

		if (!hlslSource.empty())
		{
			std::unordered_map<ShaderStage, std::string> sources;
			sources[ShaderStage::Vertex] = hlslSource;
			sources[ShaderStage::Fragment] = hlslSource;
			CompileShaders(sources);
		}
		else
		{
			ZN_CORE_ERROR("Failed to load HLSL shader file: {}", filepath);
		}
	}

	void VulkanShader::CompileShaders(const std::unordered_map<ShaderStage, std::string>& sources)
	{
		auto& dxcCompiler = DXCCompiler::Get();

		if (!dxcCompiler.Initialize())
		{
			ZN_CORE_ERROR("Failed to initialize DXC compiler for shader: {}", m_Name);
			return;
		}

		auto compiledShaders = dxcCompiler.CompileShaders(sources);

		CreateShadersFromSpirv(compiledShaders);

		if (compiledShaders.empty())
		{
			ZN_CORE_ERROR("Failed to compile any shaders for: {}", m_Name);
		}
	}

	void VulkanShader::CreateShadersFromSpirv(const std::unordered_map<ShaderStage, std::vector<uint32_t>>& spirvCode)
	{
		for (auto& [stage, spirv] : spirvCode)
		{
			if (!spirv.empty())
			{
				m_ShaderModules[stage] = CreateShaderModule(spirv);
			}
		}
	}

	std::string VulkanShader::LoadTextFile(const std::string& filepath)
	{
		std::ifstream file(filepath);
		if (!file.is_open())
		{
			ZN_CORE_TRACE("Failed to open text file: {}", filepath);
			return {};
		}

		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
		return content;
	}

	VkShaderModule VulkanShader::CreateShaderModule(const std::vector<uint32_t>& code)
	{
		if (code.empty())
		{
			ZN_CORE_ERROR("Cannot create shader module from empty SPIR-V code");
			return VK_NULL_HANDLE;
		}

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size() * sizeof(uint32_t);
		createInfo.pCode = code.data();

		VkShaderModule shaderModule;
		VK_CHECK_RESULT(vkCreateShaderModule(m_Device->GetVulkanDevice(), &createInfo, nullptr, &shaderModule));
		return shaderModule;
	}

	Ref<VulkanShader> VulkanShader::Create(const std::string& filepath)
	{
		return Ref<VulkanShader>::Create(filepath);
	}

	Ref<VulkanShader> VulkanShader::Create(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
	{
		return Ref<VulkanShader>::Create(name, vertexSource, fragmentSource);
	}

	Ref<VulkanShader> VulkanShader::CreateFromSpirv(const std::string& name,
													const std::vector<uint32_t>& vertexSpirv,
													const std::vector<uint32_t>& fragmentSpirv)
	{
		auto shader = Ref<VulkanShader>::Create();
		shader->m_Name = name;
		shader->m_Device = VulkanContext::GetCurrentDevice();

		std::unordered_map<ShaderStage, std::vector<uint32_t>> spirvCode;
		if (!vertexSpirv.empty())
			spirvCode[ShaderStage::Vertex] = vertexSpirv;
		if (!fragmentSpirv.empty())
			spirvCode[ShaderStage::Fragment] = fragmentSpirv;

		shader->CreateShadersFromSpirv(spirvCode);

		return shader;
	}
}