#include "znpch.hpp"
#include "DXCCompiler.hpp"
#include "VulkanShader.hpp"

namespace Zenith {

	DXCCompiler::DXCCompiler() = default;
	DXCCompiler::~DXCCompiler() = default;

	bool DXCCompiler::Initialize()
	{
		if (m_Initialized)
			return true;

		try {
#ifdef _WIN32
			HRESULT hr;

			hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_Utils.GetAddressOf()));
			if (FAILED(hr))
			{
				ZN_CORE_ERROR("Failed to create DXC utils: 0x{:x}", hr);
				return false;
			}

			hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_Compiler.GetAddressOf()));
			if (FAILED(hr))
			{
				ZN_CORE_ERROR("Failed to create DXC compiler: 0x{:x}", hr);
				return false;
			}

			hr = m_Utils->CreateDefaultIncludeHandler(m_IncludeHandler.GetAddressOf());
			if (FAILED(hr))
			{
				ZN_CORE_ERROR("Failed to create DXC include handler: 0x{:x}", hr);
				return false;
			}
#else
			HRESULT hr;

			hr = DxcCreateInstance(CLSID_DxcUtils, __uuidof(IDxcUtils), reinterpret_cast<void**>(&m_Utils));
			if (FAILED(hr))
			{
				ZN_CORE_ERROR("Failed to create DXC utils on Linux: 0x{:x}", hr);
				return false;
			}

			hr = DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), reinterpret_cast<void**>(&m_Compiler));
			if (FAILED(hr))
			{
				ZN_CORE_ERROR("Failed to create DXC compiler on Linux: 0x{:x}", hr);
				return false;
			}

			hr = m_Utils->CreateDefaultIncludeHandler(&m_IncludeHandler);
			if (FAILED(hr))
			{
				ZN_CORE_ERROR("Failed to create DXC include handler on Linux: 0x{:x}", hr);
				return false;
			}
#endif

			m_Initialized = true;
			return true;
		}
		catch (const std::exception& e)
		{
			ZN_CORE_ERROR("Exception during DXC initialization: {}", e.what());
			return false;
		}
		catch (...)
		{
			ZN_CORE_ERROR("Unknown exception during DXC initialization");
			return false;
		}
	}

	void DXCCompiler::Shutdown()
	{
		PlatformCleanup();
		m_Initialized = false;
	}

	void DXCCompiler::PlatformCleanup()
	{
#ifdef _WIN32
		m_IncludeHandler.Reset();
		m_Compiler.Reset();
		m_Utils.Reset();
#else
		if (m_IncludeHandler) {
			m_IncludeHandler->Release();
			m_IncludeHandler = nullptr;
		}
		if (m_Compiler) {
			m_Compiler->Release();
			m_Compiler = nullptr;
		}
		if (m_Utils) {
			m_Utils->Release();
			m_Utils = nullptr;
		}
#endif
	}

	ShaderCompileResult DXCCompiler::CompileShader(
		const std::string& source,
		const std::string& entryPoint,
		const std::string& profile,
		const std::string& filename)
	{
		ShaderCompileResult result;

		if (!m_Initialized)
		{
			result.errorMessage = "DXC Compiler not initialized";
			return result;
		}

		try {
#ifdef _WIN32
			Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob;
			HRESULT hr = m_Utils->CreateBlob(source.c_str(), source.length(), CP_UTF8, &sourceBlob);
#else
			IDxcBlobEncoding* sourceBlob = nullptr;
			HRESULT hr = m_Utils->CreateBlob(source.c_str(), source.length(), 65001, &sourceBlob); // CP_UTF8 = 65001
#endif
			if (FAILED(hr))
			{
				result.errorMessage = "Failed to create source blob";
				return result;
			}

			std::wstring wEntryPoint(entryPoint.begin(), entryPoint.end());
			std::wstring wProfile(profile.begin(), profile.end());

			std::vector<LPCWSTR> arguments = {
				L"-spirv",                    // Generate SPIR-V
				L"-fspv-target-env=vulkan1.2", // Target Vulkan 1.2
				L"-E", wEntryPoint.c_str(),   // Entry point (still "main")
				L"-T", wProfile.c_str(),      // Shader profile
				L"-O3",                       // Optimization level
			};

			if (profile == "vs_6_0") {
				arguments.push_back(L"-D");
				arguments.push_back(L"VERTEX_SHADER=1");
			} else if (profile == "ps_6_0") {
				arguments.push_back(L"-D");
				arguments.push_back(L"FRAGMENT_SHADER=1");
			}

			DxcBuffer sourceBuffer;
			sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
			sourceBuffer.Size = sourceBlob->GetBufferSize();
			sourceBuffer.Encoding = 0;

#ifdef _WIN32
			Microsoft::WRL::ComPtr<IDxcResult> compileResult;
			hr = m_Compiler->Compile(
				&sourceBuffer,
				arguments.data(),
				static_cast<uint32_t>(arguments.size()),
				m_IncludeHandler.Get(),
				IID_PPV_ARGS(&compileResult)
			);
#else
			IDxcResult* compileResult = nullptr;
			hr = m_Compiler->Compile(
				&sourceBuffer,
				arguments.data(),
				static_cast<uint32_t>(arguments.size()),
				m_IncludeHandler,
				__uuidof(IDxcResult),
				reinterpret_cast<void**>(&compileResult)
			);
#endif

			if (FAILED(hr))
			{
				result.errorMessage = "DXC compilation failed";
#ifndef _WIN32
				if (sourceBlob) sourceBlob->Release();
#endif
				return result;
			}

			HRESULT compileStatus;
			compileResult->GetStatus(&compileStatus);

			if (FAILED(compileStatus))
			{
#ifdef _WIN32
				Microsoft::WRL::ComPtr<IDxcBlobEncoding> errorBlob;
				hr = compileResult->GetErrorBuffer(&errorBlob);
#else
				IDxcBlobEncoding* errorBlob = nullptr;
				hr = compileResult->GetErrorBuffer(&errorBlob);
#endif
				if (SUCCEEDED(hr) && errorBlob)
				{
					result.errorMessage = std::string(
						static_cast<const char*>(errorBlob->GetBufferPointer()),
						errorBlob->GetBufferSize()
					);
#ifndef _WIN32
					errorBlob->Release();
#endif
				}
				else
				{
					result.errorMessage = "Unknown compilation error";
				}
#ifndef _WIN32
				if (sourceBlob) sourceBlob->Release();
				if (compileResult) compileResult->Release();
#endif
				return result;
			}

#ifdef _WIN32
			Microsoft::WRL::ComPtr<IDxcBlob> bytecodeBlob;
			hr = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&bytecodeBlob), nullptr);
#else
			IDxcBlob* bytecodeBlob = nullptr;
			hr = compileResult->GetOutput(DXC_OUT_OBJECT, __uuidof(IDxcBlob), reinterpret_cast<void**>(&bytecodeBlob), nullptr);
#endif
			if (FAILED(hr) || !bytecodeBlob)
			{
				result.errorMessage = "Failed to get compiled bytecode";
#ifndef _WIN32
				if (sourceBlob) sourceBlob->Release();
				if (compileResult) compileResult->Release();
#endif
				return result;
			}

			const uint32_t* spirvData = static_cast<const uint32_t*>(bytecodeBlob->GetBufferPointer());
			size_t spirvSize = bytecodeBlob->GetBufferSize() / sizeof(uint32_t);

			result.spirvBytecode.assign(spirvData, spirvData + spirvSize);
			result.success = true;

#ifndef _WIN32
			// Manual cleanup for Linux
			if (bytecodeBlob) bytecodeBlob->Release();
			if (sourceBlob) sourceBlob->Release();
			if (compileResult) compileResult->Release();
#endif

			return result;
		}
		catch (const std::exception& e)
		{
			result.errorMessage = std::string("Exception during compilation: ") + e.what();
			return result;
		}
		catch (...)
		{
			result.errorMessage = "Unknown exception during compilation";
			return result;
		}
	}

	std::unordered_map<ShaderStage, std::vector<uint32_t>> DXCCompiler::CompileShaders(
		const std::unordered_map<ShaderStage, std::string>& sources)
	{
		std::unordered_map<ShaderStage, std::vector<uint32_t>> compiledShaders;

		for (const auto& [stage, source] : sources)
		{
			std::string profile = GetProfileForStage(stage);
			std::string entryPoint = GetEntryPointForStage(stage);

			auto result = CompileShader(source, entryPoint, profile);
			if (result.success)
			{
				compiledShaders[stage] = std::move(result.spirvBytecode);
			}
			else
			{
				ZN_CORE_ERROR("Failed to compile {} shader: {}", profile, result.errorMessage);
			}
		}

		return compiledShaders;
	}

	std::string DXCCompiler::GetProfileForStage(ShaderStage stage)
	{
		switch (stage)
		{
		case ShaderStage::Vertex:   return "vs_6_0";
		case ShaderStage::Fragment: return "ps_6_0";
		case ShaderStage::Geometry: return "gs_6_0";
		case ShaderStage::Compute:  return "cs_6_0";
		default:
			ZN_CORE_ASSERT(false, "Unsupported shader stage");
			return "";
		}
	}

	std::string DXCCompiler::GetEntryPointForStage(ShaderStage stage)
	{
		return "main";
	}

	DXCCompiler& DXCCompiler::Get()
	{
		static DXCCompiler instance;
		return instance;
	}

}