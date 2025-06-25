#include "znpch.hpp"
#include "Image.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanImage.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<Image2D> Image2D::Create(const ImageSpecification& specification, Buffer buffer)
	{
		ZN_CORE_VERIFY(!buffer);

		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanImage2D>::Create(specification);
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<ImageView> ImageView::Create(const ImageViewSpecification& specification)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanImageView>::Create(specification);
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
