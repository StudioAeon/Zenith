#include "znpch.hpp"
#include "Shader.hpp"

#include <utility>

#include "Zenith/Renderer/Renderer.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanShader.hpp"

#if ZN_HAS_SHADER_COMPILER
#include "Zenith/Renderer/API/Vulkan/ShaderCompiler/VulkanShaderCompiler.hpp"
#endif

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<Shader> Shader::Create(const std::string& filepath, bool forceCompile, bool disableOptimization)
	{
		Ref<Shader> result = nullptr;

		result = Ref<VulkanShader>::Create(filepath, forceCompile, disableOptimization);
		return result;
	}

	Ref<Shader> Shader::CreateFromString(const std::string& source)
	{
		Ref<Shader> result = nullptr;
		return result;
	}

	ShaderLibrary::ShaderLibrary()
	{
	}

	ShaderLibrary::~ShaderLibrary()
	{
	}

	void ShaderLibrary::Add(const Zenith::Ref<Shader>& shader)
	{
		auto& name = shader->GetName();
		ZN_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Load(std::string_view path, bool forceCompile, bool disableOptimization)
	{
		Ref<Shader> shader;
		if (!forceCompile)
		{
			// Try compile from source
			// Unavailable at runtime
#if ZN_HAS_SHADER_COMPILER
			shader = VulkanShaderCompiler::Compile(path, forceCompile, disableOptimization);
#endif
		}

		auto& name = shader->GetName();
		ZN_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Load(std::string_view name, const std::string& path)
	{
		ZN_CORE_ASSERT(m_Shaders.find(std::string(name)) == m_Shaders.end());
		m_Shaders[std::string(name)] = Shader::Create(path);
	}

	const Ref<Shader>& ShaderLibrary::Get(const std::string& name) const
	{
		ZN_CORE_ASSERT(m_Shaders.find(name) != m_Shaders.end());
		return m_Shaders.at(name);
	}

	ShaderUniform::ShaderUniform(std::string name, const ShaderUniformType type, const uint32_t size, const uint32_t offset)
		: m_Name(std::move(name)), m_Type(type), m_Size(size), m_Offset(offset)
	{
	}

	constexpr std::string_view ShaderUniform::UniformTypeToString(const ShaderUniformType type)
	{
		if (type == ShaderUniformType::Bool)
		{
			return std::string("Boolean");
		}
		else if (type == ShaderUniformType::Int)
		{
			return std::string("Int");
		}
		else if (type == ShaderUniformType::Float)
		{
			return std::string("Float");
		}

		return std::string("None");
	}

}