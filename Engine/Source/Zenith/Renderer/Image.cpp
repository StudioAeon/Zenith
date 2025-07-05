#include "znpch.hpp"
#include "Image.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanImage.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<Image2D> Image2D::Create(const ImageSpecification& specification, Buffer buffer)
	{
		ZN_CORE_VERIFY(!buffer);

		return Ref<VulkanImage2D>::Create(specification);
	}

	Ref<ImageView> ImageView::Create(const ImageViewSpecification& specification)
	{
		return Ref<VulkanImageView>::Create(specification);
	}

}
