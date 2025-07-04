#include "znpch.hpp"
#include "VulkanShaderCompiler.hpp"

#include "VulkanShaderCache.hpp"

#include "ShaderPreprocessing/GlslIncluder.hpp"
#include "ShaderPreprocessing/HlslIncluder.hpp"
#include "ShaderPreprocessing/IncludePathManager.hpp"

#include "Zenith/Core/Hash.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanContext.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanShader.hpp"
#include "Zenith/Serialization/FileStream.hpp"
#include "Zenith/Utilities/StringUtils.hpp"
#include "Zenith/Utilities/FileSystem.hpp"

#include <dxc/dxcapi.h>
#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>
#include <spirv-tools/libspirv.h>

#include <filesystem>
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

		static ShaderUniformType SPIRVReflectTypeToShaderUniformType(const SpvReflectTypeDescription& type)
		{
			if (type.type_flags & SPV_REFLECT_TYPE_FLAG_BOOL)
				return ShaderUniformType::Bool;

			if (type.type_flags & SPV_REFLECT_TYPE_FLAG_INT)
			{
				switch (type.traits.numeric.vector.component_count)
				{
					case 1: return ShaderUniformType::Int;
					case 2: return ShaderUniformType::IVec2;
					case 3: return ShaderUniformType::IVec3;
					case 4: return ShaderUniformType::IVec4;
				}
			}

			if (type.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
			{
				if (type.traits.numeric.matrix.column_count == 3)
					return ShaderUniformType::Mat3;
				if (type.traits.numeric.matrix.column_count == 4)
					return ShaderUniformType::Mat4;

				switch (type.traits.numeric.vector.component_count)
				{
					case 1: return ShaderUniformType::Float;
					case 2: return ShaderUniformType::Vec2;
					case 3: return ShaderUniformType::Vec3;
					case 4: return ShaderUniformType::Vec4;
				}
			}

			ZN_CORE_ASSERT(false, "Unknown SPIRV-Reflect type!");
			return ShaderUniformType::None;
		}
	}

	VulkanShaderCompiler::VulkanShaderCompiler(const std::filesystem::path& shaderSourcePath, bool disableOptimization)
		: m_ShaderSourcePath(shaderSourcePath), m_DisableOptimization(disableOptimization)
	{
		m_Language = ShaderUtils::ShaderLangFromExtension(shaderSourcePath.extension().string());
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

		ZN_CORE_TRACE_TAG("Renderer", "Compiling shader: {}", m_ShaderSourcePath.string());
		m_ShaderSource = PreProcess(source);
		const VkShaderStageFlagBits changedStages = VulkanShaderCache::HasChanged(this);

		bool compileSucceeded = CompileOrGetVulkanBinaries(m_SPIRVDebugData, m_SPIRVData, changedStages, forceCompile);
		if (!compileSucceeded)
		{
			ZN_CORE_ASSERT(false);
			return false;
		}

		// Reflection using spirv-reflect
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
		switch (m_Language)
		{
			case ShaderUtils::SourceLang::GLSL: return PreProcessGLSL(source);
			case ShaderUtils::SourceLang::HLSL: return PreProcessHLSL(source);
		}

		ZN_CORE_VERIFY(false);
		return {};
	}

	std::map<VkShaderStageFlagBits, std::string> VulkanShaderCompiler::PreProcessGLSL(const std::string& source)
	{
		std::map<VkShaderStageFlagBits, std::string> shaderSources = ShaderPreprocessor::PreprocessShader<ShaderUtils::SourceLang::GLSL>(source, m_AcknowledgedMacros);

		static shaderc::Compiler compiler;

		// Create include path manager instead of shaderc_util::FileFinder
		Utils::IncludePathManager includeManager;
		includeManager.AddSearchPath("Resources/Shaders/Include/GLSL/");   // Main include directory
		includeManager.AddSearchPath("Resources/Shaders/Include/Common/"); // Shared include directory

		for (auto& [stage, shaderSource] : shaderSources)
		{
			shaderc::CompileOptions options;
			options.AddMacroDefinition("__GLSL__");
			options.AddMacroDefinition(std::string(ShaderUtils::VKStageToShaderMacro(stage)));

			const auto& globalMacros = Renderer::GetGlobalShaderMacros();
			for (const auto& [name, value] : globalMacros)
				options.AddMacroDefinition(name, value);

			// Create GlslIncluder with our custom path manager
			GlslIncluder* includer = new GlslIncluder(&includeManager);

			options.SetIncluder(std::unique_ptr<GlslIncluder>(includer));
			const auto preProcessingResult = compiler.PreprocessGlsl(shaderSource, ShaderUtils::ShaderStageToShaderC(stage), m_ShaderSourcePath.string().c_str(), options);
			if (preProcessingResult.GetCompilationStatus() != shaderc_compilation_status_success)
				ZN_CORE_ERROR_TAG("Renderer", std::format("Failed to pre-process \"{}\"'s {} shader.\nError: {}", m_ShaderSourcePath.string(), ShaderUtils::ShaderStageToString(stage), preProcessingResult.GetErrorMessage()));

			m_StagesMetadata[stage].HashValue = Hash::GenerateFNVHash(shaderSource);
			m_StagesMetadata[stage].Headers = std::move(includer->GetIncludeData());

			m_AcknowledgedMacros.merge(includer->GetParsedSpecialMacros());

			shaderSource = std::string(preProcessingResult.begin(), preProcessingResult.end());
		}
		return shaderSources;
	}

	std::map<VkShaderStageFlagBits, std::string> VulkanShaderCompiler::PreProcessHLSL(const std::string& source)
	{
		std::map<VkShaderStageFlagBits, std::string> shaderSources = ShaderPreprocessor::PreprocessShader<ShaderUtils::SourceLang::HLSL>(source, m_AcknowledgedMacros);

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

		if (m_Language == ShaderUtils::SourceLang::GLSL)
		{
			static shaderc::Compiler compiler;
			shaderc::CompileOptions shaderCOptions;
			shaderCOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
			shaderCOptions.SetWarningsAsErrors();
			if (options.GenerateDebugInfo)
				shaderCOptions.SetGenerateDebugInfo();

			if (options.Optimize)
				shaderCOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

			// Compile shader
			const shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(stageSource, ShaderUtils::ShaderStageToShaderC(stage), m_ShaderSourcePath.string().c_str(), shaderCOptions);

			if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				return std::format("{}While compiling shader file: {} \nAt stage: {}", module.GetErrorMessage(), m_ShaderSourcePath.string(), ShaderUtils::ShaderStageToString(stage));

			outputBinary = std::vector<uint32_t>(module.begin(), module.end());
			return {}; // Success
		}
		else if (m_Language == ShaderUtils::SourceLang::HLSL)
		{
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

			char tempfileName[] = "zenith-hlsl-XXXXXX.spv";
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

			// TODO: Error handling
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
		}
		return "Unknown language!";
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
				// Disable optimization for compute shaders because of shaderc internal error
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
		ZN_CORE_TRACE_TAG("Renderer", " SPIRV-Reflect Shader Reflection");
		ZN_CORE_TRACE_TAG("Renderer", "===========================");

		// Create SPIRV-Reflect module
		SpvReflectShaderModule module;
		SpvReflectResult result = spvReflectCreateShaderModule(shaderData.size() * sizeof(uint32_t), shaderData.data(), &module);
		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			ZN_CORE_ERROR_TAG("Renderer", "Failed to create SPIRV-Reflect module!");
			return;
		}

		// Enumerate descriptor sets
		uint32_t descriptorSetCount = 0;
		result = spvReflectEnumerateDescriptorSets(&module, &descriptorSetCount, nullptr);
		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			ZN_CORE_ERROR_TAG("Renderer", "Failed to enumerate descriptor sets!");
			spvReflectDestroyShaderModule(&module);
			return;
		}

		std::vector<SpvReflectDescriptorSet*> descriptorSets(descriptorSetCount);
		result = spvReflectEnumerateDescriptorSets(&module, &descriptorSetCount, descriptorSets.data());

		// Process descriptor sets
		for (auto* descriptorSet : descriptorSets)
		{
			uint32_t setIndex = descriptorSet->set;

			if (setIndex >= m_ReflectionData.ShaderDescriptorSets.size())
				m_ReflectionData.ShaderDescriptorSets.resize(setIndex + 1);

			ShaderResource::ShaderDescriptorSet& shaderDescriptorSet = m_ReflectionData.ShaderDescriptorSets[setIndex];

			for (uint32_t i = 0; i < descriptorSet->binding_count; ++i)
			{
				const SpvReflectDescriptorBinding& binding = *descriptorSet->bindings[i];

				switch (binding.descriptor_type)
				{
					case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					{
						ZN_CORE_TRACE_TAG("Renderer", "Uniform Buffer: {} (set={}, binding={})", binding.name, setIndex, binding.binding);

						if (s_UniformBuffers[setIndex].find(binding.binding) == s_UniformBuffers[setIndex].end())
						{
							ShaderResource::UniformBuffer uniformBuffer;
							uniformBuffer.BindingPoint = binding.binding;
							uniformBuffer.Size = binding.block.size;
							uniformBuffer.Name = binding.name;
							uniformBuffer.ShaderStage = VK_SHADER_STAGE_ALL;
							s_UniformBuffers[setIndex][binding.binding] = uniformBuffer;
						}
						else
						{
							auto& uniformBuffer = s_UniformBuffers[setIndex][binding.binding];
							if (binding.block.size > uniformBuffer.Size)
								uniformBuffer.Size = binding.block.size;
						}
						shaderDescriptorSet.UniformBuffers[binding.binding] = s_UniformBuffers[setIndex][binding.binding];
						break;
					}
					case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
					{
						ZN_CORE_TRACE_TAG("Renderer", "Storage Buffer: {} (set={}, binding={})", binding.name, setIndex, binding.binding);

						if (s_StorageBuffers[setIndex].find(binding.binding) == s_StorageBuffers[setIndex].end())
						{
							ShaderResource::StorageBuffer storageBuffer;
							storageBuffer.BindingPoint = binding.binding;
							storageBuffer.Size = binding.block.size;
							storageBuffer.Name = binding.name;
							storageBuffer.ShaderStage = VK_SHADER_STAGE_ALL;
							s_StorageBuffers[setIndex][binding.binding] = storageBuffer;
						}
						else
						{
							auto& storageBuffer = s_StorageBuffers[setIndex][binding.binding];
							if (binding.block.size > storageBuffer.Size)
								storageBuffer.Size = binding.block.size;
						}
						shaderDescriptorSet.StorageBuffers[binding.binding] = s_StorageBuffers[setIndex][binding.binding];
						break;
					}
					case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
					{
						ZN_CORE_TRACE_TAG("Renderer", "Combined Image Sampler: {} (set={}, binding={})", binding.name, setIndex, binding.binding);

						auto& imageSampler = shaderDescriptorSet.ImageSamplers[binding.binding];
						imageSampler.BindingPoint = binding.binding;
						imageSampler.DescriptorSet = setIndex;
						imageSampler.Name = binding.name;
						imageSampler.ShaderStage = shaderStage;
						imageSampler.Dimension = binding.image.dim;
						imageSampler.ArraySize = binding.count;

						m_ReflectionData.Resources[binding.name] = ShaderResourceDeclaration(binding.name, setIndex, binding.binding, binding.count);
						break;
					}
					case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
					{
						ZN_CORE_TRACE_TAG("Renderer", "Separate Image: {} (set={}, binding={})", binding.name, setIndex, binding.binding);

						auto& separateTexture = shaderDescriptorSet.SeparateTextures[binding.binding];
						separateTexture.BindingPoint = binding.binding;
						separateTexture.DescriptorSet = setIndex;
						separateTexture.Name = binding.name;
						separateTexture.ShaderStage = shaderStage;
						separateTexture.Dimension = binding.image.dim;
						separateTexture.ArraySize = binding.count;

						m_ReflectionData.Resources[binding.name] = ShaderResourceDeclaration(binding.name, setIndex, binding.binding, binding.count);
						break;
					}
					case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
					{
						ZN_CORE_TRACE_TAG("Renderer", "Separate Sampler: {} (set={}, binding={})", binding.name, setIndex, binding.binding);

						auto& separateSampler = shaderDescriptorSet.SeparateSamplers[binding.binding];
						separateSampler.BindingPoint = binding.binding;
						separateSampler.DescriptorSet = setIndex;
						separateSampler.Name = binding.name;
						separateSampler.ShaderStage = shaderStage;
						separateSampler.ArraySize = binding.count;

						m_ReflectionData.Resources[binding.name] = ShaderResourceDeclaration(binding.name, setIndex, binding.binding, binding.count);
						break;
					}
					case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					{
						ZN_CORE_TRACE_TAG("Renderer", "Storage Image: {} (set={}, binding={})", binding.name, setIndex, binding.binding);

						auto& storageImage = shaderDescriptorSet.StorageImages[binding.binding];
						storageImage.BindingPoint = binding.binding;
						storageImage.DescriptorSet = setIndex;
						storageImage.Name = binding.name;
						storageImage.ShaderStage = shaderStage;
						storageImage.Dimension = binding.image.dim;
						storageImage.ArraySize = binding.count;

						m_ReflectionData.Resources[binding.name] = ShaderResourceDeclaration(binding.name, setIndex, binding.binding, binding.count);
						break;
					}
				}
			}
		}

		// Enumerate push constants
		uint32_t pushConstantCount = 0;
		result = spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, nullptr);
		if (result == SPV_REFLECT_RESULT_SUCCESS && pushConstantCount > 0)
		{
			std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
			result = spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, pushConstants.data());

			for (auto* pushConstant : pushConstants)
			{
				ZN_CORE_TRACE_TAG("Renderer", "Push Constant: {} (size={})", pushConstant->name, pushConstant->size);

				uint32_t bufferOffset = 0;
				if (!m_ReflectionData.PushConstantRanges.empty())
					bufferOffset = m_ReflectionData.PushConstantRanges.back().Offset + m_ReflectionData.PushConstantRanges.back().Size;

				auto& pushConstantRange = m_ReflectionData.PushConstantRanges.emplace_back();
				pushConstantRange.ShaderStage = shaderStage;
				pushConstantRange.Size = pushConstant->size - bufferOffset;
				pushConstantRange.Offset = bufferOffset;

				// Skip empty push constant buffers
				if (!pushConstant->name || strlen(pushConstant->name) == 0 || strcmp(pushConstant->name, "u_Renderer") == 0)
					continue;

				ShaderBuffer& buffer = m_ReflectionData.ConstantBuffers[pushConstant->name];
				buffer.Name = pushConstant->name;
				buffer.Size = pushConstant->size - bufferOffset;

				// Process push constant members
				for (uint32_t i = 0; i < pushConstant->member_count; ++i)
				{
					const SpvReflectBlockVariable& member = pushConstant->members[i];
					std::string uniformName = std::format("{}.{}", pushConstant->name, member.name);

					// Convert SPIRV-Reflect type to engine type
					ShaderUniformType uniformType = Utils::SPIRVReflectTypeToShaderUniformType(*member.type_description);

					buffer.Uniforms[uniformName] = ShaderUniform(uniformName, uniformType, member.size, member.offset - bufferOffset);
				}
			}
		}

		ZN_CORE_TRACE_TAG("Renderer", "Special macros:");
		for (const auto& macro : m_AcknowledgedMacros)
		{
			ZN_CORE_TRACE_TAG("Renderer", "  {0}", macro);
		}

		ZN_CORE_TRACE_TAG("Renderer", "===========================");

		// Clean up
		spvReflectDestroyShaderModule(&module);
	}

}