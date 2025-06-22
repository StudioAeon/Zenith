#pragma once

#include "Zenith/Core/Base.hpp"
#include <vector>
#include <string>
#include <unordered_map>

#ifdef _WIN32
	#include <dxcapi.h>
	#include <wrl/client.h>
	using Microsoft::WRL::ComPtr;
#else
#include <dxcapi.h>

template<typename T>
using ComPtr = T*;

#define IID_PPV_ARGS(ppv) __uuidof(**(ppv)), reinterpret_cast<void**>(ppv)
#endif

namespace Zenith {

	enum class ShaderStage;

	struct ShaderCompileResult
	{
		std::vector<uint32_t> spirvBytecode;
		std::string errorMessage;
		bool success = false;
	};

	class DXCCompiler
	{
	public:
		DXCCompiler();
		~DXCCompiler();

		bool Initialize();
		void Shutdown();

		ShaderCompileResult CompileShader(
			const std::string& source,
			const std::string& entryPoint,
			const std::string& profile,
			const std::string& filename = ""
		);

		std::unordered_map<ShaderStage, std::vector<uint32_t>> CompileShaders(
			const std::unordered_map<ShaderStage, std::string>& sources
		);

		static DXCCompiler& Get();

	private:
		std::string GetProfileForStage(ShaderStage stage);
		std::string GetEntryPointForStage(ShaderStage stage);

		void PlatformCleanup();

	private:
		ComPtr<IDxcUtils> m_Utils;
		ComPtr<IDxcCompiler3> m_Compiler;
		ComPtr<IDxcIncludeHandler> m_IncludeHandler;
		bool m_Initialized = false;
	};

}