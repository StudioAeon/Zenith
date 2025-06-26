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

		// TODO: Implement DXC type conversion when reflection is implemented
		// This function will be needed when we implement proper DXC reflection
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
		std::vector<const wchar_t*> arguments{ buffer.c_str(), L"-E", L"main", L"-T",ShaderUtils::HLSLShaderProfile(stage), L"-spirv", L"-fspv-target-env=vulkan1.2",
			DXC_ARG_PACK_MATRIX_COLUMN_MAJOR, DXC_ARG_WARNINGS_ARE_ERRORS

			// TODO: L"-fspv-reflect" causes a validation error about SPV_GOOGLE_hlsl_functionality1
			// Without this argument, not much info will be in Nsight.
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
		// Note(Emily): This is *atrocious* but dxc's integration refuses to process builtin HLSL without ICE'ing
		//				from the integration.

		char tempfileName[] = "Zenith-hlsl-XXXXXX.spv";
		int outfile = mkstemps(tempfileName, 4);

		std::string dxc = std::format("{}/bin/dxc", FileSystem::GetEnvironmentVariable("VULKAN_SDK"));
		std::string sourcePath = m_ShaderSourcePath.string();

		std::vector<const char*> exec{
			dxc.c_str(),
			sourcePath.c_str(),

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

		// TODO(Emily): Error handling
		pid_t pid;
		posix_spawnattr_t attr;
		posix_spawnattr_init(&attr);

		std::string ld_lib_path = std::format("LD_LIBRARY_PATH={}", getenv("LD_LIBRARY_PATH"));
		char* env[] = { ld_lib_path.data(), NULL };
		if (posix_spawn(&pid, exec[0], NULL, &attr, (char**)exec.data(), env))
		{
			return std::format("Could not execute `{}` for shader compilation: {} {}", exec[0], m_ShaderSourcePath.string(), ShaderUtils::ShaderStageToString(stage));
		}
		int status;
		waitpid(pid, &status, 0);

		if (WEXITSTATUS(status))
		{
			return std::format("Compilation failed\nWhile compiling shader file: {} \nAt stage: {}", m_ShaderSourcePath.string(), ShaderUtils::ShaderStageToString(stage));
		}

		off_t size = lseek(outfile, 0, SEEK_END);
		lseek(outfile, 0, SEEK_SET);
		outputBinary.resize(size / sizeof(uint32_t));
		read(outfile, outputBinary.data(), size);
		close(outfile);
		unlink(tempfileName);

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

	void VulkanShaderCompiler::Reflect(VkShaderStageFlagBits shaderStage, const std::vector<uint32_t>& shaderData)
	{
		ZN_CORE_TRACE_TAG("Renderer", "===========================");
		ZN_CORE_TRACE_TAG("Renderer", " DXC Shader Reflection");
		ZN_CORE_TRACE_TAG("Renderer", "===========================");

		// TODO: Implement DXC-based reflection instead of spirv-cross
		// For now, we'll use a basic implementation that matches the structure
		// This would need to be implemented using DXC's reflection APIs

		ZN_CORE_WARN_TAG("Renderer", "DXC reflection not yet implemented - using placeholder");

		// Placeholder implementation - you'll need to implement proper DXC reflection here
		// The reflection should use DXC's IDxcContainerReflection and related interfaces
		// to extract uniform buffers, storage buffers, push constants, and other resources

		ZN_CORE_TRACE_TAG("Renderer", "Special macros:");
		for (const auto& macro : m_AcknowledgedMacros)
		{
			ZN_CORE_TRACE_TAG("Renderer", "  {0}", macro);
		}

		ZN_CORE_TRACE_TAG("Renderer", "===========================");
	}

}