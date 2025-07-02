#include "znpch.hpp"
#include "VulkanShaderCompiler.hpp"

#include "VulkanShaderCache.hpp"

#include "ShaderPreprocessing/HlslIncluder.hpp"

#include "Zenith/Core/Hash.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanContext.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanShader.hpp"
#include "Zenith/Serialization/FileStream.hpp"
#include "Zenith/Utilities/StringUtils.hpp"
#include "Zenith/Utilities/FileSystem.hpp"

#include <dxc/dxcapi.h>
#include <spirv_reflect.h>

#include <cstdlib>
#include <format>

#if defined(ZN_PLATFORM_LINUX)
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace Zenith {

	static std::unordered_map<uint32_t, std::unordered_map<uint32_t, ShaderResource::UniformBuffer>> s_UniformBuffers; // set -> binding point -> buffer
	static std::unordered_map<uint32_t, std::unordered_map<uint32_t, ShaderResource::StorageBuffer>> s_StorageBuffers; // set -> binding point -> buffer

	namespace Utils {

		static const char* GetCacheDirectory()
		{
			// TODO: make sure the assets directory is valid
			return "Resources/Cache/Shader/Vulkan";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::string cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}
	}

	VulkanShaderCompiler::VulkanShaderCompiler(const std::filesystem::path& shaderSourcePath, bool disableOptimization)
		: m_ShaderSourcePath(shaderSourcePath), m_DisableOptimization(disableOptimization)
	{
		// Only HLSL is supported now
		ZN_CORE_VERIFY(shaderSourcePath.extension() == ".hlsl", "Only HLSL shaders are supported!");
	}

	bool VulkanShaderCompiler::Reload(bool forceCompile)
	{
		m_ShaderSource.clear();
		m_StagesMetadata.clear();
		m_SPIRVDebugData.clear();
		m_SPIRVData.clear();

		Utils::CreateCacheDirectoryIfNeeded();
		const std::string source = Utils::ReadFileAndSkipBOM(m_ShaderSourcePath);
		ZN_CORE_VERIFY(source.size(), "Failed to load shader!");

		ZN_CORE_TRACE_TAG("Renderer", "Compiling HLSL shader: {}", m_ShaderSourcePath.string());
		m_ShaderSource = PreProcess(source);
		const VkShaderStageFlagBits changedStages = VulkanShaderCache::HasChanged(this);

		bool compileSucceeded = CompileOrGetVulkanBinaries(m_SPIRVDebugData, m_SPIRVData, changedStages, forceCompile);
		if (!compileSucceeded)
		{
			ZN_CORE_ASSERT(false);
			return false;
		}

		// Reflection
		if (forceCompile || changedStages || !TryReadCachedReflectionData())
		{
			ReflectAllShaderStages(m_SPIRVDebugData);
			SerializeReflectionData();
		}

		return true;
	}

	void VulkanShaderCompiler::ClearUniformBuffers()
	{
		s_UniformBuffers.clear();
		s_StorageBuffers.clear();
	}

	std::map<VkShaderStageFlagBits, std::string> VulkanShaderCompiler::PreProcess(const std::string& source)
	{
		// Only HLSL preprocessing now
		std::map<VkShaderStageFlagBits, std::string> shaderSources = ShaderPreprocessor::PreprocessShader(source, m_AcknowledgedMacros);

#ifdef ZN_PLATFORM_WINDOWS
		std::wstring buffer = m_ShaderSourcePath.wstring();
#else
		std::wstring buffer;
		buffer.resize(m_ShaderSourcePath.string().size() * 2);
		mbstowcs(buffer.data(), m_ShaderSourcePath.string().c_str(), m_ShaderSourcePath.string().size());
#endif

		std::vector<const wchar_t*> arguments{ buffer.c_str(), L"-P", DXC_ARG_WARNINGS_ARE_ERRORS,
			L"-I Resources/Shaders/Include/Common/",
			L"-I Resources/Shaders/Include/HLSL/", //Main include directory
			L"-D", L"__HLSL__",
		};

		const auto& globalMacros = Renderer::GetGlobalShaderMacros();
		for (const auto& [name, value] : globalMacros)
		{
			arguments.emplace_back(L"-D");
			arguments.push_back(nullptr);
			std::string def;
			if (value.size())
				def = std::format("{}={}", name, value);
			else
				def = name;

			wchar_t* def_buffer = new wchar_t[def.size() + 1];
			mbstowcs(def_buffer, def.c_str(), def.size());
			def_buffer[def.size()] = 0;
			arguments[arguments.size() - 1] = def_buffer;
		}

		if (!DxcInstances::Compiler)
		{
#ifdef ZN_PLATFORM_WINDOWS
			DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DxcInstances::Compiler));
			DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&DxcInstances::Utils));
#endif
		}

		for (auto& [stage, shaderSource] : shaderSources)
		{
#ifdef ZN_PLATFORM_WINDOWS
			IDxcBlobEncoding* pSource;
			DxcInstances::Utils->CreateBlob(shaderSource.c_str(), (uint32_t)shaderSource.size(), CP_UTF8, &pSource);

			DxcBuffer sourceBuffer;
			sourceBuffer.Ptr = pSource->GetBufferPointer();
			sourceBuffer.Size = pSource->GetBufferSize();
			sourceBuffer.Encoding = 0;

			const std::unique_ptr<HlslIncluder> includer = std::make_unique<HlslIncluder>();
			IDxcResult* pCompileResult;
			HRESULT err = DxcInstances::Compiler->Compile(&sourceBuffer, arguments.data(), (uint32_t)arguments.size(), includer.get(), IID_PPV_ARGS(&pCompileResult));

			// Error Handling
			std::string error;
			const bool failed = FAILED(err);
			if (failed)
				error = std::format("Failed to pre-process, Error: {}\n", err);
			IDxcBlobEncoding* pErrors = nullptr;
			pCompileResult->GetErrorBuffer(&pErrors);
			if (pErrors->GetBufferPointer() && pErrors->GetBufferSize())
				error.append(std::format("{}\nWhile pre-processing shader file: {} \nAt stage: {}", (char*)pErrors->GetBufferPointer(), m_ShaderSourcePath.string(), ShaderUtils::ShaderStageToString(stage)));

			if (error.empty())
			{
				// Successful compilation
				IDxcBlob* pResult;
				pCompileResult->GetResult(&pResult);

				const size_t size = pResult->GetBufferSize();
				shaderSource = (const char*)pResult->GetBufferPointer();
				pResult->Release();
			}
			else
			{
				ZN_CORE_ERROR_TAG("Renderer", error);
			}

			m_StagesMetadata[stage].HashValue = Hash::GenerateFNVHash(shaderSource);
			m_StagesMetadata[stage].Headers = std::move(includer->GetIncludeData());

			m_AcknowledgedMacros.merge(includer->GetParsedSpecialMacros());
#else
			m_StagesMetadata[stage] = StageData{};
#endif
		}
		return shaderSources;
	}

	std::string VulkanShaderCompiler::Compile(std::vector<uint32_t>& outputBinary, const VkShaderStageFlagBits stage, CompilationOptions options) const
	{
		const std::string& stageSource = m_ShaderSource.at(stage);

#ifdef ZN_PLATFORM_WINDOWS
		std::wstring buffer = m_ShaderSourcePath.wstring();
		std::vector<const wchar_t*> arguments{
		buffer.c_str(),
		L"-E", L"main",
		L"-T", ShaderUtils::HLSLShaderProfile(stage),
		L"-spirv",
		L"-fspv-target-env=vulkan1.2",
		DXC_ARG_PACK_MATRIX_COLUMN_MAJOR,
		DXC_ARG_WARNINGS_ARE_ERRORS,

		L"-fvk-use-dx-layout",     // Ensures correct memory layout matching HLSL
		//L"-fvk-bind-register",
		//L"-fspv-reflect",
		};

		if (options.GenerateDebugInfo)
		{
			arguments.emplace_back(L"-Qembed_debug");
			arguments.emplace_back(DXC_ARG_DEBUG);
		}

		if (stage & (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_GEOMETRY_BIT))
			arguments.push_back(L"-fvk-invert-y");

		IDxcBlobEncoding* pSource;
		DxcInstances::Utils->CreateBlob(stageSource.c_str(), (uint32_t)stageSource.size(), CP_UTF8, &pSource);

		DxcBuffer sourceBuffer;
		sourceBuffer.Ptr = pSource->GetBufferPointer();
		sourceBuffer.Size = pSource->GetBufferSize();
		sourceBuffer.Encoding = 0;

		IDxcResult* pCompileResult;
		std::string error;

		HRESULT err = DxcInstances::Compiler->Compile(&sourceBuffer, arguments.data(), (uint32_t)arguments.size(), nullptr, IID_PPV_ARGS(&pCompileResult));

		// Error Handling
		const bool failed = FAILED(err);
		if (failed)
			error = std::format("Failed to compile, Error: {}\n", err);
		IDxcBlobUtf8* pErrors;
		pCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), NULL);
		if (pErrors && pErrors->GetStringLength() > 0)
			error.append(std::format("{}\nWhile compiling shader file: {} \nAt stage: {}", (char*)pErrors->GetBufferPointer(), m_ShaderSourcePath.string(), ShaderUtils::ShaderStageToString(stage)));

		if (error.empty())
		{
			// Successful compilation
			IDxcBlob* pResult;
			pCompileResult->GetResult(&pResult);

			const size_t size = pResult->GetBufferSize();
			outputBinary.resize(size / sizeof(uint32_t));
			std::memcpy(outputBinary.data(), pResult->GetBufferPointer(), size);
			pResult->Release();
		}
		pCompileResult->Release();
		pSource->Release();

		return error;
#elif defined(ZN_PLATFORM_LINUX)
		char tempSourceName[] = "Zenith-hlsl-src-XXXXXX.hlsl";
		int srcFile = mkstemps(tempSourceName, 5);
		if (srcFile == -1) {
			return std::format("Failed to create temporary source file: {}", strerror(errno));
		}

		if (write(srcFile, stageSource.c_str(), stageSource.size()) == -1) {
			close(srcFile);
			unlink(tempSourceName);
			return std::format("Failed to write temporary source file: {}", strerror(errno));
		}
		close(srcFile);

		char tempfileName[] = "Zenith-hlsl-XXXXXX.spv";
		int outfile = mkstemps(tempfileName, 4);
		if (outfile == -1) {
			unlink(tempSourceName);
			return std::format("Failed to create temporary output file: {}", strerror(errno));
		}

#ifdef DXC_EXECUTABLE_PATH
		std::string dxc = DXC_EXECUTABLE_PATH;
#else
	std::string dxc = "dxc";
#endif

		std::vector<const char*> exec{
			dxc.c_str(),
			tempSourceName,

			"-E", "main",
			"-T", ShaderUtils::HLSLShaderProfile(stage),
			"-spirv",
			"-fspv-target-env=vulkan1.2",
			"-Zpc",
			"-WX",

			"-I", "Resources/Shaders/Include/Common",
			"-I", "Resources/Shaders/Include/HLSL",

			"-Fo", tempfileName
		};

		if (options.GenerateDebugInfo)
		{
			exec.push_back("-Qembed_debug");
			exec.push_back("-Zi");
		}

		if (stage & (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_GEOMETRY_BIT))
			exec.push_back("-fvk-invert-y");

		exec.push_back(NULL);

		std::vector<std::string> env_strings;
		std::vector<char*> env_ptrs;

		const char* existing_ld_path = getenv("LD_LIBRARY_PATH");
		if (existing_ld_path) {
			env_strings.push_back(std::format("LD_LIBRARY_PATH={}", existing_ld_path));
		} else {
			env_strings.push_back("LD_LIBRARY_PATH=");
		}

		for (auto& str : env_strings) {
			env_ptrs.push_back(str.data());
		}
		env_ptrs.push_back(nullptr);

		pid_t pid;
		posix_spawnattr_t attr;
		posix_spawnattr_init(&attr);

		if (posix_spawn(&pid, exec[0], NULL, &attr, (char**)exec.data(), env_ptrs.data()))
		{
			close(outfile);
			unlink(tempfileName);
			unlink(tempSourceName);
			return std::format("Could not execute `{}` for shader compilation: {} {}", exec[0], tempSourceName, ShaderUtils::ShaderStageToString(stage));
		}

		int status;
		waitpid(pid, &status, 0);

		if (WEXITSTATUS(status))
		{
			close(outfile);
			unlink(tempfileName);
			unlink(tempSourceName);
			return std::format("Compilation failed\nWhile compiling shader file: {} \nAt stage: {}", tempSourceName, ShaderUtils::ShaderStageToString(stage));
		}

		off_t size = lseek(outfile, 0, SEEK_END);
		lseek(outfile, 0, SEEK_SET);
		outputBinary.resize(size / sizeof(uint32_t));
		read(outfile, outputBinary.data(), size);
		close(outfile);

		unlink(tempfileName);
		unlink(tempSourceName);

		return {};
#endif
		return "Platform not supported for HLSL compilation!";
	}

	Ref<VulkanShader> VulkanShaderCompiler::Compile(const std::filesystem::path& shaderSourcePath, bool forceCompile, bool disableOptimization)
	{
		// Set name
		std::string path = shaderSourcePath.string();
		size_t found = path.find_last_of("/\\");
		std::string name = found != std::string::npos ? path.substr(found + 1) : path;
		found = name.find_last_of('.');
		name = found != std::string::npos ? name.substr(0, found) : name;

		Ref<VulkanShader> shader = Ref<VulkanShader>::Create();
		shader->m_AssetPath = shaderSourcePath;
		shader->m_Name = name;
		shader->m_DisableOptimization = disableOptimization;

		Ref<VulkanShaderCompiler> compiler = Ref<VulkanShaderCompiler>::Create(shaderSourcePath, disableOptimization);
		compiler->Reload(forceCompile);

		shader->LoadAndCreateShaders(compiler->GetSPIRVData());
		shader->SetReflectionData(compiler->m_ReflectionData);
		shader->CreateDescriptors();

		Renderer::AcknowledgeParsedGlobalMacros(compiler->GetAcknowledgedMacros(), shader);
		Renderer::OnShaderReloaded(shader->GetHash());
		return shader;
	}

	bool VulkanShaderCompiler::TryRecompile(Ref<VulkanShader> shader)
	{
		Ref<VulkanShaderCompiler> compiler = Ref<VulkanShaderCompiler>::Create(shader->m_AssetPath, shader->m_DisableOptimization);
		bool compileSucceeded = compiler->Reload(true);
		if (!compileSucceeded)
			return false;

		shader->Release();

		shader->LoadAndCreateShaders(compiler->GetSPIRVData());
		shader->SetReflectionData(compiler->m_ReflectionData);
		shader->CreateDescriptors();

		Renderer::AcknowledgeParsedGlobalMacros(compiler->GetAcknowledgedMacros(), shader);
		Renderer::OnShaderReloaded(shader->GetHash());

		return true;
	}

	bool VulkanShaderCompiler::CompileOrGetVulkanBinaries(std::map<VkShaderStageFlagBits, std::vector<uint32_t>>& outputDebugBinary, std::map<VkShaderStageFlagBits, std::vector<uint32_t>>& outputBinary, const VkShaderStageFlagBits changedStages, const bool forceCompile)
	{
		for (auto [stage, source] : m_ShaderSource)
		{
			if (!CompileOrGetVulkanBinary(stage, outputDebugBinary[stage], true, changedStages, forceCompile))
				return false;
			if (!CompileOrGetVulkanBinary(stage, outputBinary[stage], false, changedStages, forceCompile))
				return false;
		}
		return true;
	}

	bool VulkanShaderCompiler::CompileOrGetVulkanBinary(VkShaderStageFlagBits stage, std::vector<uint32_t>& outputBinary, bool debug, VkShaderStageFlagBits changedStages, bool forceCompile)
	{
		const std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		// Compile shader with debug info so we can reflect
		const auto extension = ShaderUtils::ShaderStageCachedFileExtension(stage, debug);
		if (!forceCompile && stage & ~changedStages) // Per-stage cache is found and is unchanged
		{
			TryGetVulkanCachedBinary(cacheDirectory, extension, outputBinary);
		}

		if (outputBinary.empty())
		{
			CompilationOptions options;
			if (debug)
			{
				options.GenerateDebugInfo = true;
				options.Optimize = false;
			}
			else
			{
				options.GenerateDebugInfo = false;
				// Disable optimization for compute shaders because of DXC internal error
				options.Optimize = !m_DisableOptimization && stage != VK_SHADER_STAGE_COMPUTE_BIT;
			}

			if (std::string error = Compile(outputBinary, stage, options); error.size())
			{
				ZN_CORE_ERROR_TAG("Renderer", "{}", error);
				TryGetVulkanCachedBinary(cacheDirectory, extension, outputBinary);
				if (outputBinary.empty())
				{
					//ZN_CONSOLE_LOG_ERROR("Failed to compile shader and couldn't find a cached version.");
				}
				else
				{
					//ZN_CONSOLE_LOG_ERROR("Failed to compile {}:{} so a cached version was loaded instead.", m_ShaderSourcePath.string(), ShaderUtils::ShaderStageToString(stage));
				}
				return false;
			}
			else // Compile success
			{
				auto path = cacheDirectory / (m_ShaderSourcePath.filename().string() + extension);
				std::string cachedFilePath = path.string();

				FILE* f = fopen(cachedFilePath.c_str(), "wb");
				if (!f)
					ZN_CORE_ERROR("Failed to cache shader binary!");
				fwrite(outputBinary.data(), sizeof(uint32_t), outputBinary.size(), f);
				fclose(f);
			}
		}

		return true;
	}

	void VulkanShaderCompiler::ClearReflectionData()
	{
		m_ReflectionData.ShaderDescriptorSets.clear();
		m_ReflectionData.Resources.clear();
		m_ReflectionData.ConstantBuffers.clear();
		m_ReflectionData.PushConstantRanges.clear();
	}

	void VulkanShaderCompiler::TryGetVulkanCachedBinary(const std::filesystem::path& cacheDirectory, const std::string& extension, std::vector<uint32_t>& outputBinary) const
	{
		const auto path = cacheDirectory / (m_ShaderSourcePath.filename().string() + extension);
		const std::string cachedFilePath = path.string();

		FILE* f = fopen(cachedFilePath.data(), "rb");
		if (!f)
			return;

		fseek(f, 0, SEEK_END);
		uint64_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
		outputBinary = std::vector<uint32_t>(size / sizeof(uint32_t));
		fread(outputBinary.data(), sizeof(uint32_t), outputBinary.size(), f);
		fclose(f);
	}

	bool VulkanShaderCompiler::TryReadCachedReflectionData()
	{
		struct ReflectionFileHeader
		{
			char Header[4] = { 'Z','N','S','R' };
		} header;

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();
		const auto path = cacheDirectory / (m_ShaderSourcePath.filename().string() + ".cached_vulkan.refl");
		FileStreamReader serializer(path);
		if (!serializer)
			return false;

		serializer.ReadRaw(header);

		bool validHeader = memcmp(&header, "ZNSR", 4) == 0;
		ZN_CORE_VERIFY(validHeader);
		if (!validHeader)
			return false;

		ClearReflectionData();

		uint32_t shaderDescriptorSetCount;
		serializer.ReadRaw<uint32_t>(shaderDescriptorSetCount);

		for (uint32_t i = 0; i < shaderDescriptorSetCount; i++)
		{
			auto& descriptorSet = m_ReflectionData.ShaderDescriptorSets.emplace_back();
			serializer.ReadMap(descriptorSet.UniformBuffers);
			serializer.ReadMap(descriptorSet.StorageBuffers);
			serializer.ReadMap(descriptorSet.ImageSamplers);
			serializer.ReadMap(descriptorSet.StorageImages);
			serializer.ReadMap(descriptorSet.SeparateTextures);
			serializer.ReadMap(descriptorSet.SeparateSamplers);
			serializer.ReadMap(descriptorSet.WriteDescriptorSets);
		}

		serializer.ReadMap(m_ReflectionData.Resources);
		serializer.ReadMap(m_ReflectionData.ConstantBuffers);
		serializer.ReadArray(m_ReflectionData.PushConstantRanges);

		return true;
	}

	void VulkanShaderCompiler::SerializeReflectionData()
	{
		struct ReflectionFileHeader
		{
			char Header[4] = { 'Z','N','S','R' };
		} header;

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();
		const auto path = cacheDirectory / (m_ShaderSourcePath.filename().string() + ".cached_vulkan.refl");
		FileStreamWriter serializer(path);
		serializer.WriteRaw(header);
		SerializeReflectionData(&serializer);
	}

	void VulkanShaderCompiler::SerializeReflectionData(StreamWriter* serializer)
	{
		serializer->WriteRaw<uint32_t>((uint32_t)m_ReflectionData.ShaderDescriptorSets.size());
		for (const auto& descriptorSet : m_ReflectionData.ShaderDescriptorSets)
		{
			serializer->WriteMap(descriptorSet.UniformBuffers);
			serializer->WriteMap(descriptorSet.StorageBuffers);
			serializer->WriteMap(descriptorSet.ImageSamplers);
			serializer->WriteMap(descriptorSet.StorageImages);
			serializer->WriteMap(descriptorSet.SeparateTextures);
			serializer->WriteMap(descriptorSet.SeparateSamplers);
			serializer->WriteMap(descriptorSet.WriteDescriptorSets);
		}

		serializer->WriteMap(m_ReflectionData.Resources);
		serializer->WriteMap(m_ReflectionData.ConstantBuffers);
		serializer->WriteArray(m_ReflectionData.PushConstantRanges);
	}

	void VulkanShaderCompiler::ReflectAllShaderStages(const std::map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData)
	{
		ClearReflectionData();

		for (auto [stage, data] : shaderData)
		{
			Reflect(stage, data);
		}
	}

	void VulkanShaderCompiler::LogReflectionUniforms()
	{
		ZN_CORE_INFO("===== Reflection Uniforms Dump =====");

		for (const auto& [name, buffer] : m_ReflectionData.ConstantBuffers)
		{
			ZN_CORE_INFO("Constant Buffer: Name: {}, Size: {}", name, buffer.Size);

			if (buffer.Uniforms.empty())
				ZN_CORE_INFO("  (No uniforms found)");

			for (const auto& [uniformName, uniform] : buffer.Uniforms)
			{
				ZN_CORE_INFO("  Uniform: Name: {}, Size: {}, Offset: {}",
					uniformName,
					uniform.GetSize(),
					uniform.GetOffset()
				);
			}
		}

		ZN_CORE_INFO("====================================");
	}

	void VulkanShaderCompiler::Reflect(VkShaderStageFlagBits shaderStage, const std::vector<uint32_t>& shaderData)
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		ZN_CORE_INFO("===========================");
		ZN_CORE_INFO(" SPIRV-Reflect Shader Reflection");
		ZN_CORE_INFO("===========================");

		// Create reflection module
		SpvReflectShaderModule module;
		SpvReflectResult result = spvReflectCreateShaderModule(
			shaderData.size() * sizeof(uint32_t),
			shaderData.data(),
			&module
		);

		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			ZN_CORE_ERROR("Failed to create reflection module for stage: {}",
							   ShaderUtils::ShaderStageToString(shaderStage));
			return;
		}

		// Reflect Descriptor Bindings
		uint32_t descriptorBindingCount = 0;
		result = spvReflectEnumerateDescriptorBindings(&module, &descriptorBindingCount, nullptr);
		if (result == SPV_REFLECT_RESULT_SUCCESS && descriptorBindingCount > 0)
		{
			std::vector<SpvReflectDescriptorBinding*> descriptorBindings(descriptorBindingCount);
			result = spvReflectEnumerateDescriptorBindings(&module, &descriptorBindingCount, descriptorBindings.data());

			if (result == SPV_REFLECT_RESULT_SUCCESS)
			{
				for (const auto* binding : descriptorBindings)
				{
					ProcessDescriptorBinding(binding, shaderStage);
				}
			}
		}

		// Reflect Push Constants
		uint32_t pushConstantCount = 0;
		result = spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, nullptr);
		if (result == SPV_REFLECT_RESULT_SUCCESS && pushConstantCount > 0)
		{
			std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
			result = spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, pushConstants.data());

			if (result == SPV_REFLECT_RESULT_SUCCESS)
			{
				for (const auto* pushConstant : pushConstants)
				{
					ProcessPushConstant(pushConstant, shaderStage);
				}
			}
		}

		LogReflectionUniforms();

		ZN_CORE_TRACE("Found {} descriptor sets", m_ReflectionData.ShaderDescriptorSets.size());

		ZN_CORE_INFO(">>> Final Reflected Uniforms:");
		for (const auto& [bufferName, buffer] : m_ReflectionData.ConstantBuffers)
		{
			for (const auto& [uniformName, uniform] : buffer.Uniforms)
			{
				ZN_CORE_INFO("  [{}] -> {} (Size: {}, Offset: {})",
					bufferName, uniformName, uniform.GetSize(), uniform.GetOffset());
			}
		}

		// Clean up
		spvReflectDestroyShaderModule(&module);

		ZN_CORE_INFO("Special macros:");
		for (const auto& macro : m_AcknowledgedMacros)
		{
			ZN_CORE_INFO("  {0}", macro);
		}

		ZN_CORE_INFO("===========================");
	}

	void VulkanShaderCompiler::ProcessDescriptorBinding(const SpvReflectDescriptorBinding* binding, VkShaderStageFlagBits shaderStage)
	{
		const char* name = binding->name ? binding->name : "unnamed";
		uint32_t set = binding->set;
		uint32_t bindingPoint = binding->binding;

		switch (binding->descriptor_type)
		{
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			{
				ZN_CORE_INFO("Uniform Buffer: {} (set={}, binding={})", name, set, bindingPoint);

				uint32_t size = binding->block.size;

				if (set >= m_ReflectionData.ShaderDescriptorSets.size())
					m_ReflectionData.ShaderDescriptorSets.resize(set + 1);

				ShaderResource::ShaderDescriptorSet& shaderDescriptorSet = m_ReflectionData.ShaderDescriptorSets[set];

				if (s_UniformBuffers[set].find(bindingPoint) == s_UniformBuffers[set].end())
				{
					ShaderResource::UniformBuffer uniformBuffer;
					uniformBuffer.BindingPoint = bindingPoint;
					uniformBuffer.Size = size;
					uniformBuffer.Name = name;
					uniformBuffer.ShaderStage = VK_SHADER_STAGE_ALL;
					s_UniformBuffers[set][bindingPoint] = uniformBuffer;
				}
				else
				{
					ShaderResource::UniformBuffer& uniformBuffer = s_UniformBuffers[set][bindingPoint];
					if (size > uniformBuffer.Size)
						uniformBuffer.Size = size;
				}

				shaderDescriptorSet.UniformBuffers[bindingPoint] = s_UniformBuffers[set][bindingPoint];

				ShaderBuffer& buffer = m_ReflectionData.ConstantBuffers[name];
				buffer.Name = name;
				buffer.Size = size;

				buffer.Uniforms.clear();

				for (uint32_t i = 0; i < binding->block.member_count; i++)
				{
					const SpvReflectBlockVariable& member = binding->block.members[i];
					const char* memberName = member.name ? member.name : "unnamed";
					uint32_t offset = member.offset;
					uint32_t memberSize = member.size;
					ShaderUniformType uniformType = SPIRVTypeToShaderUniformType(member.type_description);

					std::string uniformName = memberName;
					buffer.Uniforms[uniformName] = ShaderUniform(uniformName, uniformType, memberSize, offset);

					ZN_CORE_INFO("  Member: {} Type: {} Size: {} Offset: {}",
						uniformName, static_cast<int>(uniformType), memberSize, offset);
				}

				ZN_CORE_INFO("  Size: {0}", size);
				ZN_CORE_INFO("-------------------");
				break;
			}

			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			{
				ZN_CORE_INFO("Storage Buffer: {} (set={}, binding={})", name, set, bindingPoint);

				uint32_t size = binding->block.size;

				if (set >= m_ReflectionData.ShaderDescriptorSets.size())
					m_ReflectionData.ShaderDescriptorSets.resize(set + 1);

				ShaderResource::ShaderDescriptorSet& shaderDescriptorSet = m_ReflectionData.ShaderDescriptorSets[set];

				if (s_StorageBuffers[set].find(bindingPoint) == s_StorageBuffers[set].end())
				{
					ShaderResource::StorageBuffer storageBuffer;
					storageBuffer.BindingPoint = bindingPoint;
					storageBuffer.Size = size;
					storageBuffer.Name = name;
					storageBuffer.ShaderStage = VK_SHADER_STAGE_ALL;
					s_StorageBuffers[set][bindingPoint] = storageBuffer;
				}
				else
				{
					ShaderResource::StorageBuffer& storageBuffer = s_StorageBuffers[set][bindingPoint];
					if (size > storageBuffer.Size)
						storageBuffer.Size = size;
				}
				shaderDescriptorSet.StorageBuffers[bindingPoint] = s_StorageBuffers[set][bindingPoint];

				ZN_CORE_INFO("  Size: {0}", size);
				ZN_CORE_INFO("-------------------");
				break;
			}

			case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			{
				ZN_CORE_INFO("Sampled Image: {} (set={}, binding={})", name, set, bindingPoint);

				uint32_t arraySize = binding->count > 0 ? binding->count : 1;

				if (set >= m_ReflectionData.ShaderDescriptorSets.size())
					m_ReflectionData.ShaderDescriptorSets.resize(set + 1);

				ShaderResource::ShaderDescriptorSet& shaderDescriptorSet = m_ReflectionData.ShaderDescriptorSets[set];
				auto& imageSampler = shaderDescriptorSet.ImageSamplers[bindingPoint];
				imageSampler.BindingPoint = bindingPoint;
				imageSampler.DescriptorSet = set;
				imageSampler.Name = name;
				imageSampler.ShaderStage = shaderStage;
				imageSampler.Dimension = binding->image.dim;
				imageSampler.ArraySize = arraySize;

				m_ReflectionData.Resources[name] = ShaderResourceDeclaration(name, set, bindingPoint, arraySize);
				break;
			}

			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			{
				ZN_CORE_INFO("Separate Image: {} (set={}, binding={})", name, set, bindingPoint);

				uint32_t arraySize = binding->count > 0 ? binding->count : 1;

				if (set >= m_ReflectionData.ShaderDescriptorSets.size())
					m_ReflectionData.ShaderDescriptorSets.resize(set + 1);

				ShaderResource::ShaderDescriptorSet& shaderDescriptorSet = m_ReflectionData.ShaderDescriptorSets[set];
				auto& imageSampler = shaderDescriptorSet.SeparateTextures[bindingPoint];
				imageSampler.BindingPoint = bindingPoint;
				imageSampler.DescriptorSet = set;
				imageSampler.Name = name;
				imageSampler.ShaderStage = shaderStage;
				imageSampler.Dimension = binding->image.dim;
				imageSampler.ArraySize = arraySize;

				m_ReflectionData.Resources[name] = ShaderResourceDeclaration(name, set, bindingPoint, arraySize);
				break;
			}

			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
			{
				ZN_CORE_INFO("Separate Sampler: {} (set={}, binding={})", name, set, bindingPoint);

				uint32_t arraySize = binding->count > 0 ? binding->count : 1;

				if (set >= m_ReflectionData.ShaderDescriptorSets.size())
					m_ReflectionData.ShaderDescriptorSets.resize(set + 1);

				ShaderResource::ShaderDescriptorSet& shaderDescriptorSet = m_ReflectionData.ShaderDescriptorSets[set];
				auto& imageSampler = shaderDescriptorSet.SeparateSamplers[bindingPoint];
				imageSampler.BindingPoint = bindingPoint;
				imageSampler.DescriptorSet = set;
				imageSampler.Name = name;
				imageSampler.ShaderStage = shaderStage;
				imageSampler.Dimension = binding->image.dim;
				imageSampler.ArraySize = arraySize;

				m_ReflectionData.Resources[name] = ShaderResourceDeclaration(name, set, bindingPoint, arraySize);
				break;
			}

			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			{
				ZN_CORE_INFO("Storage Image: {} (set={}, binding={})", name, set, bindingPoint);

				uint32_t arraySize = binding->count > 0 ? binding->count : 1;

				if (set >= m_ReflectionData.ShaderDescriptorSets.size())
					m_ReflectionData.ShaderDescriptorSets.resize(set + 1);

				ShaderResource::ShaderDescriptorSet& shaderDescriptorSet = m_ReflectionData.ShaderDescriptorSets[set];
				auto& imageSampler = shaderDescriptorSet.StorageImages[bindingPoint];
				imageSampler.BindingPoint = bindingPoint;
				imageSampler.DescriptorSet = set;
				imageSampler.Name = name;
				imageSampler.Dimension = binding->image.dim;
				imageSampler.ArraySize = arraySize;
				imageSampler.ShaderStage = shaderStage;

				m_ReflectionData.Resources[name] = ShaderResourceDeclaration(name, set, bindingPoint, arraySize);
				break;
			}

			default:
				ZN_CORE_INFO("Other Descriptor: {} (type={}, set={}, binding={})",
								   name, static_cast<int>(binding->descriptor_type), set, bindingPoint);
				break;
		}

		for (const auto& [bufferName, buffer] : m_ReflectionData.ConstantBuffers)
		{
			for (const auto& [uniformName, uniform] : buffer.Uniforms)
			{
				ZN_CORE_INFO("Reflected uniform: {}", uniformName);
			}
		}
	}

	void VulkanShaderCompiler::ProcessPushConstant(const SpvReflectBlockVariable* pushConstant, VkShaderStageFlagBits shaderStage)
	{
		const char* bufferName = pushConstant->name ? pushConstant->name : "";
		uint32_t bufferSize = pushConstant->size;
		uint32_t memberCount = pushConstant->member_count;
		uint32_t bufferOffset = 0;

		if (m_ReflectionData.PushConstantRanges.size())
			bufferOffset = m_ReflectionData.PushConstantRanges.back().Offset + m_ReflectionData.PushConstantRanges.back().Size;

		auto& pushConstantRange = m_ReflectionData.PushConstantRanges.emplace_back();
		pushConstantRange.ShaderStage = shaderStage;
		pushConstantRange.Size = bufferSize - bufferOffset;
		pushConstantRange.Offset = bufferOffset;

		ZN_CORE_INFO("Push Constant Buffer:");
		ZN_CORE_INFO("  Name: {0}", bufferName);
		ZN_CORE_INFO("  Member Count: {0}", memberCount);
		ZN_CORE_INFO("  Size: {0}", bufferSize);

		// If bufferName can be std::string_view or const char*
		std::string_view name{bufferName ? bufferName : ""};
		if (name.empty() || name == "u_Renderer")
			return;

		ShaderBuffer& buffer = m_ReflectionData.ConstantBuffers[bufferName];
		buffer.Name = bufferName;
		buffer.Size = bufferSize - bufferOffset;

		// Process push constant members
		for (uint32_t i = 0; i < memberCount; i++)
		{
			const SpvReflectBlockVariable& member = pushConstant->members[i];
			const char* memberName = member.name ? member.name : "unnamed";
			uint32_t size = member.size;
			uint32_t offset = member.offset - bufferOffset;

			ShaderUniformType uniformType = SPIRVTypeToShaderUniformType(member.type_description);

			std::string uniformName = memberName;
			buffer.Uniforms[uniformName] = ShaderUniform(uniformName, uniformType, size, offset);
		}
	}

	ShaderUniformType VulkanShaderCompiler::SPIRVTypeToShaderUniformType(const SpvReflectTypeDescription* typeDesc)
	{
		if (!typeDesc) return ShaderUniformType::None;

		switch (typeDesc->type_flags)
		{
			case SPV_REFLECT_TYPE_FLAG_BOOL:
				return ShaderUniformType::Bool;

			case SPV_REFLECT_TYPE_FLAG_INT:
				if (typeDesc->traits.numeric.vector.component_count == 1) return ShaderUniformType::Int;
				if (typeDesc->traits.numeric.vector.component_count == 2) return ShaderUniformType::IVec2;
				if (typeDesc->traits.numeric.vector.component_count == 3) return ShaderUniformType::IVec3;
				if (typeDesc->traits.numeric.vector.component_count == 4) return ShaderUniformType::IVec4;
				break;

			case SPV_REFLECT_TYPE_FLAG_FLOAT:
				if (typeDesc->traits.numeric.matrix.column_count > 1)
				{
					// Matrix types
					if (typeDesc->traits.numeric.matrix.column_count == 3 &&
						typeDesc->traits.numeric.matrix.row_count == 3)
						return ShaderUniformType::Mat3;
					if (typeDesc->traits.numeric.matrix.column_count == 4 &&
						typeDesc->traits.numeric.matrix.row_count == 4)
						return ShaderUniformType::Mat4;
				}
				else
				{
					// Vector types
					if (typeDesc->traits.numeric.vector.component_count == 1) return ShaderUniformType::Float;
					if (typeDesc->traits.numeric.vector.component_count == 2) return ShaderUniformType::Vec2;
					if (typeDesc->traits.numeric.vector.component_count == 3) return ShaderUniformType::Vec3;
					if (typeDesc->traits.numeric.vector.component_count == 4) return ShaderUniformType::Vec4;
				}
				break;
		}

		return ShaderUniformType::None;
	}

}