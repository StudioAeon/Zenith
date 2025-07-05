#pragma once

#include "Zenith/Core/Base.hpp"

#include "Framebuffer.hpp"

#include "UniformBufferSet.hpp"
#include "StorageBufferSet.hpp"
#include "Texture.hpp"
#include "Pipeline.hpp"

namespace Zenith {

	struct RenderPassSpecification
	{
		Ref<Pipeline> TargetPipeline;
		std::string DebugName;
		glm::vec4 MarkerColor;
	};

	class RenderPass : public RefCounted
	{
	public:
		virtual ~RenderPass() = default;

		virtual RenderPassSpecification& GetSpecification() = 0;
		virtual const RenderPassSpecification& GetSpecification() const = 0;

		virtual void SetInput(std::string_view name, Ref<UniformBufferSet> uniformBufferSet) = 0;
		virtual void SetInput(std::string_view name, Ref<UniformBuffer> uniformBuffer) = 0;

		virtual void SetInput(std::string_view name, Ref<StorageBufferSet> storageBufferSet) = 0;
		virtual void SetInput(std::string_view name, Ref<StorageBuffer> storageBuffer) = 0;

		virtual void SetInput(std::string_view name, Ref<Texture2D> texture) = 0;
		virtual void SetInput(std::string_view name, Ref<TextureCube> textureCube) = 0;
		virtual void SetInput(std::string_view name, Ref<Image2D> image) = 0;

		virtual Ref<Image2D> GetOutput(uint32_t index) = 0;
		virtual Ref<Image2D> GetDepthOutput() = 0;
		virtual uint32_t GetFirstSetIndex() const = 0;

		virtual Ref<Pipeline> GetPipeline() const = 0;
		virtual Ref<Framebuffer> GetTargetFramebuffer() const = 0;

		virtual bool Validate() = 0;
		virtual void Bake() = 0;
		virtual bool Baked() const = 0;
		virtual void Prepare() = 0;

		static Ref<RenderPass> Create(const RenderPassSpecification& spec);
	};

}