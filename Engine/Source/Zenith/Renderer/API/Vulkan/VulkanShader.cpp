#include "znpch.hpp"
#include "VulkanShader.hpp"
#include "VulkanContext.hpp"
#include <fstream>
#include <filesystem>

// #include <shaderc/shaderc.hpp>

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

		std::string vertPath = path.string() + ".vert.spv";
		std::string fragPath = path.string() + ".frag.spv";

		auto vertSpirv = LoadSpirvFile(vertPath);
		auto fragSpirv = LoadSpirvFile(fragPath);

		if (!vertSpirv.empty() && !fragSpirv.empty())
		{
			m_ShaderModules[ShaderStage::Vertex] = CreateShaderModule(vertSpirv);
			m_ShaderModules[ShaderStage::Fragment] = CreateShaderModule(fragSpirv);
			ZN_CORE_INFO("Loaded shader from SPIR-V files: {}", m_Name);
		}
		else
		{
			ZN_CORE_WARN("Could not load SPIR-V files for {}, trying GLSL source compilation", m_Name);

			std::string vertSourcePath = path.string() + ".vert";
			std::string fragSourcePath = path.string() + ".frag";

			std::string vertSource = LoadTextFile(vertSourcePath);
			std::string fragSource = LoadTextFile(fragSourcePath);

			if (!vertSource.empty() && !fragSource.empty())
			{
				std::unordered_map<ShaderStage, std::string> sources;
				sources[ShaderStage::Vertex] = vertSource;
				sources[ShaderStage::Fragment] = fragSource;
				CompileShaders(sources);
			}
			else
			{
				ZN_CORE_ERROR("Failed to load shader files for: {}", m_Name);
			}
		}
	}

	void VulkanShader::CompileShaders(const std::unordered_map<ShaderStage, std::string>& sources)
	{}

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

	std::vector<uint32_t> VulkanShader::LoadSpirvFile(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			ZN_CORE_TRACE("Failed to open SPIR-V file: {}", filepath);
			return {};
		}

		size_t fileSize = (size_t)file.tellg();
		if (fileSize % sizeof(uint32_t) != 0)
		{
			ZN_CORE_ERROR("SPIR-V file size is not a multiple of 4 bytes: {}", filepath);
			file.close();
			return {};
		}

		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		file.seekg(0);
		file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
		file.close();

		return buffer;
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