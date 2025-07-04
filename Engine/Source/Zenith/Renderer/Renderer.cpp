#include "znpch.hpp"
#include "Renderer.hpp"

#include "Shader.hpp"

#include "RendererAPI.hpp"

#include "Zenith/Core/Timer.hpp"
#include "Zenith/Debug/Profiler.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanContext.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanRenderer.hpp"
#include "Zenith/Project/Project.hpp"

#include <filesystem>
#include <format>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

namespace std {
	template<>
	struct hash<Zenith::WeakRef<Zenith::Shader>>
	{
		size_t operator()(const Zenith::WeakRef<Zenith::Shader>& shader) const noexcept
		{
			return shader->GetHash();
		}
	};
}

namespace Zenith {

	Application* Renderer::s_Application = nullptr;
	static std::unordered_map<size_t, Ref<Pipeline>> s_PipelineCache;
	static RendererAPI* s_RendererAPI = nullptr;

	struct ShaderDependencies
	{
		std::vector<Ref<Pipeline>> Pipelines;
		std::vector<Ref<Material>> Materials;
	};
	static std::unordered_map<size_t, ShaderDependencies> s_ShaderDependencies;
	static std::shared_mutex s_ShaderDependenciesMutex; // ShaderDependencies can be accessed (and modified) from multiple threads, hence require synchronization

	struct GlobalShaderInfo
	{
		// Macro name, set of shaders with that macro.
		std::unordered_map<std::string, std::unordered_map<size_t, WeakRef<Shader>>> ShaderGlobalMacrosMap;
		// Shaders waiting to be reloaded.
		std::unordered_set<WeakRef<Shader>> DirtyShaders;
	};
	static GlobalShaderInfo s_GlobalShaderInfo;

	void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Pipeline> pipeline)
	{
		std::scoped_lock lock(s_ShaderDependenciesMutex);
		s_ShaderDependencies[shader->GetHash()].Pipelines.push_back(pipeline);
	}

	void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Material> material)
	{
		std::scoped_lock lock(s_ShaderDependenciesMutex);
		s_ShaderDependencies[shader->GetHash()].Materials.push_back(material);
	}

	void Renderer::OnShaderReloaded(size_t hash)
	{
		ShaderDependencies dependencies;
		{
			std::shared_lock lock(s_ShaderDependenciesMutex);
			if (auto it = s_ShaderDependencies.find(hash); it != s_ShaderDependencies.end())
			{
				dependencies = it->second; // expensive to copy, but we need to release the lock (in particular to avoid potential deadlock if things like material->OnShaderReloaded() happen to ask for the lock)
			}
		}
		for (auto& pipeline : dependencies.Pipelines)
		{
			pipeline->Invalidate();
		}
	}

	uint32_t Renderer::RT_GetCurrentFrameIndex()
	{
		// Swapchain owns the Render Thread frame index
		return s_Application->GetWindow().GetSwapChain().GetCurrentBufferIndex();
	}

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return s_Application->GetCurrentFrameIndex();
	}

	void RendererAPI::SetAPI(RendererAPIType api)
	{
		// TODO: make sure this is called at a valid time
		ZN_CORE_VERIFY(api == RendererAPIType::Vulkan, "Vulkan is currently the only supported Renderer API");
		s_CurrentRendererAPI = api;
	}

	struct RendererData
	{
		Ref<ShaderLibrary> m_ShaderLibrary;

		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BlackTexture;
		Ref<Texture2D> BRDFLutTexture;
		Ref<Texture2D> HilbertLut;
		Ref<TextureCube> BlackCubeTexture;

		std::unordered_map<std::string, std::string> GlobalShaderMacros;
	};

	static RendererConfig s_Config;
	static RendererData* s_Data = nullptr;
	constexpr static uint32_t s_RenderCommandQueueCount = 2;
	static RenderCommandQueue* s_CommandQueue[s_RenderCommandQueueCount];
	static std::atomic<uint32_t> s_RenderCommandQueueSubmissionIndex = 0;
	static RenderCommandQueue s_ResourceFreeQueue[3];

	static RendererAPI* InitRendererAPI()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::Vulkan: return znew VulkanRenderer();
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void Renderer::Init(Application* app)
	{
		s_Application = app;
		s_Data = znew RendererData();
		s_CommandQueue[0] = znew RenderCommandQueue();
		s_CommandQueue[1] = znew RenderCommandQueue();

		Renderer::SetCurrentContext(app->GetWindow().GetRenderContext());

		// Make sure we don't have more frames in flight than swapchain images
		s_Config.FramesInFlight = glm::min<uint32_t>(s_Config.FramesInFlight, app->GetWindow().GetSwapChain().GetImageCount());

		s_RendererAPI = InitRendererAPI();

		s_Data->m_ShaderLibrary = Ref<ShaderLibrary>::Create();

		Renderer::GetShaderLibrary()->Load("Resources/Shaders/BasicMesh.glsl");

		Renderer::GetApplication()->GetRenderThread().Pump();

		uint32_t whiteTextureData = 0xffffffff;
		TextureSpecification spec;
		spec.Format = ImageFormat::RGBA;
		spec.Width = 1;
		spec.Height = 1;
		s_Data->WhiteTexture = Texture2D::Create(spec, Buffer(&whiteTextureData, sizeof(uint32_t)));

		constexpr uint32_t blackTextureData = 0xff000000;
		s_Data->BlackTexture = Texture2D::Create(spec, Buffer(&blackTextureData, sizeof(uint32_t)));

		{
			TextureSpecification spec;
			spec.SamplerWrap = TextureWrap::Clamp;
			s_Data->BRDFLutTexture = Texture2D::Create(spec, std::filesystem::path("Resources/Renderer/BRDF_LUT.png"));
		}

		constexpr uint32_t blackCubeTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
		s_Data->BlackCubeTexture = TextureCube::Create(spec, Buffer(blackCubeTextureData, sizeof(blackCubeTextureData)));

		// Hilbert look-up texture! It's a 64 x 64 uint16 texture
		{
			TextureSpecification spec;
			spec.Format = ImageFormat::RED16UI;
			spec.Width = 64;
			spec.Height = 64;
			spec.SamplerWrap = TextureWrap::Clamp;
			spec.SamplerFilter = TextureFilter::Nearest;

			constexpr auto HilbertIndex = [](uint32_t posX, uint32_t posY)
			{
				uint16_t index = 0u;
				for (uint16_t curLevel = 64 / 2u; curLevel > 0u; curLevel /= 2u)
				{
					const uint16_t regionX = (posX & curLevel) > 0u;
					const uint16_t regionY = (posY & curLevel) > 0u;
					index += curLevel * curLevel * ((3u * regionX) ^ regionY);
					if (regionY == 0u)
					{
						if (regionX == 1u)
						{
							posX = uint16_t((64 - 1u)) - posX;
							posY = uint16_t((64 - 1u)) - posY;
						}

						std::swap(posX, posY);
					}
				}
				return index;
			};

			uint16_t* data = new uint16_t[(size_t)(64 * 64)];
			for (int x = 0; x < 64; x++)
			{
				for (int y = 0; y < 64; y++)
				{
					const uint16_t r2index = HilbertIndex(x, y);
					ZN_CORE_ASSERT(r2index < 65536);
					data[x + 64 * y] = r2index;
				}
			}
			s_Data->HilbertLut = Texture2D::Create(spec, Buffer(data, 1));
			delete[] data;

		}

		s_RendererAPI->Init();
	}

	void Renderer::Shutdown()
	{
		s_RendererAPI->Shutdown();

		delete s_Data;

		// Resource release queue
		for (uint32_t i = 0; i < s_Config.FramesInFlight; i++)
		{
			auto& queue = Renderer::GetRenderResourceReleaseQueue(i);
			queue.Execute();
		}

		delete s_CommandQueue[0];
		delete s_CommandQueue[1];
	}

	RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	void Renderer::RenderThreadFunc(RenderThread* renderThread)
	{
		ZN_PROFILE_THREAD("Render Thread");

		Renderer::SetCurrentContext(s_Application->GetWindow().GetRenderContext());

		while (renderThread->IsRunning())
		{
			WaitAndRender(renderThread);
		}
	}

	void Renderer::WaitAndRender(RenderThread* renderThread)
	{
		ZN_PROFILE_FUNC();

		if (!renderThread->IsRunning()) {
			return;
		}
		auto& performanceTimers = Renderer::s_Application->GetPerformanceTimers();

		// Wait for kick, then set render thread to busy
		{
			ZN_PROFILE_SCOPE("Wait");
			Timer waitTimer;
			renderThread->WaitAndSet(RenderThread::State::Kick, RenderThread::State::Busy);

			if (!renderThread->IsRunning()) {
				renderThread->Set(RenderThread::State::Idle);
				return;
			}

			performanceTimers.RenderThreadWaitTime = waitTimer.ElapsedMillis();
		}

		Timer workTimer;
		if (renderThread->IsRunning()) {
			s_CommandQueue[GetRenderQueueIndex()]->Execute();
		}

		// Rendering has completed, set state to idle
		renderThread->Set(RenderThread::State::Idle);

		performanceTimers.RenderThreadWorkTime = workTimer.ElapsedMillis();
	}

	void Renderer::SwapQueues()
	{
		s_RenderCommandQueueSubmissionIndex = (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}

	uint32_t Renderer::GetRenderQueueIndex()
	{
		return (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}

	uint32_t Renderer::GetRenderQueueSubmissionIndex()
	{
		return s_RenderCommandQueueSubmissionIndex;
	}

	void Renderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear)
	{
		ZN_CORE_ASSERT(renderPass, "RenderPass cannot be null!");

		s_RendererAPI->BeginRenderPass(renderCommandBuffer, renderPass, explicitClear);
	}

	void Renderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		s_RendererAPI->EndRenderPass(renderCommandBuffer);
	}

	void Renderer::InsertGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& color)
	{
		s_RendererAPI->InsertGPUPerfMarker(renderCommandBuffer, label, color);
	}

	void Renderer::BeginGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& markerColor)
	{
		s_RendererAPI->BeginGPUPerfMarker(renderCommandBuffer, label, markerColor);
	}

	void Renderer::EndGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		s_RendererAPI->EndGPUPerfMarker(renderCommandBuffer);
	}

	void Renderer::RT_InsertGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& color)
	{
		s_RendererAPI->RT_InsertGPUPerfMarker(renderCommandBuffer, label, color);
	}

	void Renderer::RT_BeginGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& markerColor)
	{
		s_RendererAPI->RT_BeginGPUPerfMarker(renderCommandBuffer, label, markerColor);
	}

	void Renderer::RT_EndGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		s_RendererAPI->RT_EndGPUPerfMarker(renderCommandBuffer);
	}

	void Renderer::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		s_RendererAPI->EndFrame();
	}

	void Renderer::RenderStaticMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<StaticMesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialTable> materialTable, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount)
	{
		s_RendererAPI->RenderStaticMesh(renderCommandBuffer, pipeline, mesh, meshSource, submeshIndex, materialTable, transformBuffer, transformOffset, instanceCount);
	}

	void Renderer::RenderStaticMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<StaticMesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount, Ref<Material> material, Buffer additionalUniforms)
	{
		s_RendererAPI->RenderStaticMeshWithMaterial(renderCommandBuffer, pipeline, mesh, meshSource, submeshIndex, material, transformBuffer, transformOffset, instanceCount, additionalUniforms);
	}

	void Renderer::RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform)
	{
		s_RendererAPI->RenderQuad(renderCommandBuffer, pipeline, material, transform);
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount /*= 0*/)
	{
		s_RendererAPI->RenderGeometry(renderCommandBuffer, pipeline, material, vertexBuffer, indexBuffer, transform, indexCount);
	}

	void Renderer::ClearImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> image, const ImageClearValue& clearValue, ImageSubresourceRange subresourceRange)
	{
		s_RendererAPI->ClearImage(renderCommandBuffer, image, clearValue, subresourceRange);
	}

	void Renderer::CopyImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		s_RendererAPI->CopyImage(renderCommandBuffer, sourceImage, destinationImage);
	}

	void Renderer::BlitImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		s_RendererAPI->BlitImage(renderCommandBuffer, sourceImage, destinationImage);
	}

	void Renderer::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material)
	{
		s_RendererAPI->SubmitFullscreenQuad(renderCommandBuffer, pipeline, material);
	}

	void Renderer::SubmitFullscreenQuadWithOverrides(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides)
	{
		s_RendererAPI->SubmitFullscreenQuadWithOverrides(renderCommandBuffer, pipeline, material, vertexShaderOverrides, fragmentShaderOverrides);
	}

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_Data->WhiteTexture;
	}

	Ref<Texture2D> Renderer::GetBlackTexture()
	{
		return s_Data->BlackTexture;
	}

	Ref<Texture2D> Renderer::GetHilbertLut()
	{
		return s_Data->HilbertLut;
	}

	Ref<Texture2D> Renderer::GetBRDFLutTexture()
	{
		return s_Data->BRDFLutTexture;
	}

	Ref<TextureCube> Renderer::GetBlackCubeTexture()
	{
		return s_Data->BlackCubeTexture;
	}

	RenderCommandQueue& Renderer::GetRenderCommandQueue()
	{
		return *s_CommandQueue[s_RenderCommandQueueSubmissionIndex];
	}

	RenderCommandQueue& Renderer::GetRenderResourceReleaseQueue(uint32_t index)
	{
		return s_ResourceFreeQueue[index];
	}

	const std::unordered_map<std::string, std::string>& Renderer::GetGlobalShaderMacros()
	{
		return s_Data->GlobalShaderMacros;
	}

	RendererConfig& Renderer::GetConfig()
	{
		return s_Config;
	}

	void Renderer::SetConfig(const RendererConfig& config)
	{
		s_Config = config;
	}

	void Renderer::AcknowledgeParsedGlobalMacros(const std::unordered_set<std::string>& macros, Ref<Shader> shader)
	{
		for (const std::string& macro : macros)
		{
			s_GlobalShaderInfo.ShaderGlobalMacrosMap[macro][shader->GetHash()] = shader;
		}
	}

	void Renderer::SetMacroInShader(Ref<Shader> shader, const std::string& name, const std::string& value)
	{
		shader->SetMacro(name, value);
		s_GlobalShaderInfo.DirtyShaders.emplace(shader.Raw());
	}

	void Renderer::SetGlobalMacroInShaders(const std::string& name, const std::string& value)
	{
		if (s_Data->GlobalShaderMacros.find(name) != s_Data->GlobalShaderMacros.end())
		{
			if (s_Data->GlobalShaderMacros.at(name) == value)
				return;
		}

		s_Data->GlobalShaderMacros[name] = value;

		if (s_GlobalShaderInfo.ShaderGlobalMacrosMap.find(name) == s_GlobalShaderInfo.ShaderGlobalMacrosMap.end())
		{
			ZN_CORE_WARN_TAG("Renderer", "No shaders with {} macro found", name);
			return;
		}

		ZN_CORE_ASSERT(s_GlobalShaderInfo.ShaderGlobalMacrosMap.find(name) != s_GlobalShaderInfo.ShaderGlobalMacrosMap.end(), "Macro has not been passed from any shader!");
		for (auto& [hash, shader] : s_GlobalShaderInfo.ShaderGlobalMacrosMap.at(name))
		{
			ZN_CORE_ASSERT(shader.IsValid(), "Shader is deleted!");
			s_GlobalShaderInfo.DirtyShaders.emplace(shader);
		}
	}

	bool Renderer::UpdateDirtyShaders()
	{
		// TODO: how is this going to work for dist?
		const bool updatedAnyShaders = s_GlobalShaderInfo.DirtyShaders.size();
		for (WeakRef<Shader> shader : s_GlobalShaderInfo.DirtyShaders)
		{
			ZN_CORE_ASSERT(shader.IsValid(), "Shader is deleted!");
			shader->RT_Reload(true);
		}
		s_GlobalShaderInfo.DirtyShaders.clear();

		return updatedAnyShaders;
	}

	GPUMemoryStats Renderer::GetGPUMemoryStats()
	{
		return VulkanAllocator::GetStats();
	}

}
