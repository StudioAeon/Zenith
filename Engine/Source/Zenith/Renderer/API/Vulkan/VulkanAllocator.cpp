#include "znpch.hpp"
#include "VulkanAllocator.hpp"

#include "VulkanContext.hpp"

#include "Zenith/Utilities/StringUtils.hpp"

#if ZN_LOG_RENDERER_ALLOCATIONS
#define ZN_ALLOCATOR_LOG(...) ZN_CORE_TRACE(__VA_ARGS__)
#else
#define ZN_ALLOCATOR_LOG(...)
#endif

#define ZN_GPU_TRACK_MEMORY_ALLOCATION 1

namespace Zenith {

	struct VulkanAllocatorData
	{
		VmaAllocator Allocator;
		uint64_t TotalAllocatedBytes = 0;
		
		uint64_t MemoryUsage = 0; // all heaps
	};

	enum class AllocationType : uint8_t
	{
		None = 0, Buffer = 1, Image = 2
	};

	static VulkanAllocatorData* s_Data = nullptr;
	struct AllocInfo
	{
		uint64_t AllocatedSize = 0;
		AllocationType Type = AllocationType::None;
	};
	static std::map<VmaAllocation, AllocInfo> s_AllocationMap;

	VulkanAllocator::VulkanAllocator(const std::string& tag)
		: m_Tag(tag)
	{
	}

	VulkanAllocator::~VulkanAllocator()
	{
	}

	VmaAllocation VulkanAllocator::AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage usage, VkBuffer& outBuffer)
	{
		ZN_CORE_VERIFY(bufferCreateInfo.size > 0);

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateBuffer(s_Data->Allocator, &bufferCreateInfo, &allocCreateInfo, &outBuffer, &allocation, nullptr);
		if (allocation == nullptr)
		{
			ZN_CORE_ERROR_TAG("Renderer", "Failed to allocate GPU buffer!");
			ZN_CORE_ERROR_TAG("Renderer", "  Requested size: {}", Utils::BytesToString(bufferCreateInfo.size));
			auto stats = GetStats();
			ZN_CORE_ERROR_TAG("Renderer", "  GPU mem usage: {}/{}", Utils::BytesToString(stats.Used), Utils::BytesToString(stats.TotalAvailable));
		}

		// TODO: Tracking
		VmaAllocationInfo allocInfo{};
		vmaGetAllocationInfo(s_Data->Allocator, allocation, &allocInfo);
		ZN_ALLOCATOR_LOG("VulkanAllocator ({0}): allocating buffer; size = {1}", m_Tag, Utils::BytesToString(allocInfo.size));

		{
			s_Data->TotalAllocatedBytes += allocInfo.size;
			ZN_ALLOCATOR_LOG("VulkanAllocator ({0}): total allocated since start is {1}", m_Tag, Utils::BytesToString(s_Data->TotalAllocatedBytes));
		}

#if ZN_GPU_TRACK_MEMORY_ALLOCATION
		auto& allocTrack = s_AllocationMap[allocation];
		allocTrack.AllocatedSize = allocInfo.size;
		allocTrack.Type = AllocationType::Buffer;
		s_Data->MemoryUsage += allocInfo.size;
#endif

		return allocation;
	}

	VmaAllocation VulkanAllocator::AllocateImage(VkImageCreateInfo imageCreateInfo, VmaMemoryUsage usage, VkImage& outImage, VkDeviceSize* allocatedSize)
	{
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateImage(s_Data->Allocator, &imageCreateInfo, &allocCreateInfo, &outImage, &allocation, nullptr);
		if (allocation == nullptr)
		{
			ZN_CORE_ERROR_TAG("Renderer", "Failed to allocate GPU image!");
			ZN_CORE_ERROR_TAG("Renderer", "  Requested size: {}x{}x{}", imageCreateInfo.extent.width, imageCreateInfo.extent.height, imageCreateInfo.extent.depth);
			ZN_CORE_ERROR_TAG("Renderer", "  Mips: {}", imageCreateInfo.mipLevels);
			ZN_CORE_ERROR_TAG("Renderer", "  Layers: {}", imageCreateInfo.arrayLayers);
			auto stats = GetStats();
			ZN_CORE_ERROR_TAG("Renderer", "  GPU mem usage: {}/{}", Utils::BytesToString(stats.Used), Utils::BytesToString(stats.TotalAvailable));
		}

		// TODO: Tracking
		VmaAllocationInfo allocInfo;
		vmaGetAllocationInfo(s_Data->Allocator, allocation, &allocInfo);
		if (allocatedSize)
			*allocatedSize = allocInfo.size;
		ZN_ALLOCATOR_LOG("VulkanAllocator ({0}): allocating image; size = {1}", m_Tag, Utils::BytesToString(allocInfo.size));

		{
			s_Data->TotalAllocatedBytes += allocInfo.size;
			ZN_ALLOCATOR_LOG("VulkanAllocator ({0}): total allocated since start is {1}", m_Tag, Utils::BytesToString(s_Data->TotalAllocatedBytes));
		}

#if ZN_GPU_TRACK_MEMORY_ALLOCATION
		auto& allocTrack = s_AllocationMap[allocation];
		allocTrack.AllocatedSize = allocInfo.size;
		allocTrack.Type = AllocationType::Image;
		s_Data->MemoryUsage += allocInfo.size;
#endif

		return allocation;
	}

	void VulkanAllocator::Free(VmaAllocation allocation)
	{
		vmaFreeMemory(s_Data->Allocator, allocation);

#if ZN_GPU_TRACK_MEMORY_ALLOCATION
		auto it = s_AllocationMap.find(allocation);
		if (it != s_AllocationMap.end())
		{
			s_Data->MemoryUsage -= it->second.AllocatedSize;
			s_AllocationMap.erase(it);
		}
		else
		{
			ZN_CORE_ERROR("Could not find GPU memory allocation: {}", (void*)allocation);
		}
#endif
	}

	void VulkanAllocator::DestroyImage(VkImage image, VmaAllocation allocation)
	{
		ZN_CORE_ASSERT(image);
		ZN_CORE_ASSERT(allocation);
		vmaDestroyImage(s_Data->Allocator, image, allocation);

#if ZN_GPU_TRACK_MEMORY_ALLOCATION
		auto it = s_AllocationMap.find(allocation);
		if (it != s_AllocationMap.end())
		{
			s_Data->MemoryUsage -= it->second.AllocatedSize;
			s_AllocationMap.erase(it);
		}
		else
		{
			ZN_CORE_ERROR("Could not find GPU memory allocation: {}", (void*)allocation);
		}
#endif
	}

	void VulkanAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
	{
		ZN_CORE_ASSERT(buffer);
		ZN_CORE_ASSERT(allocation);
		vmaDestroyBuffer(s_Data->Allocator, buffer, allocation);

#if ZN_GPU_TRACK_MEMORY_ALLOCATION
		auto it = s_AllocationMap.find(allocation);
		if (it != s_AllocationMap.end())
		{
			s_Data->MemoryUsage -= it->second.AllocatedSize;
			s_AllocationMap.erase(it);
		}
		else
		{
			ZN_CORE_ERROR("Could not find GPU memory allocation: {}", (void*)allocation);
		}
#endif
	}

	void VulkanAllocator::UnmapMemory(VmaAllocation allocation)
	{
		vmaUnmapMemory(s_Data->Allocator, allocation);
	}

	void VulkanAllocator::DumpStats()
	{
		const auto& memoryProps = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetMemoryProperties();
		std::vector<VmaBudget> budgets(memoryProps.memoryHeapCount);

		vmaGetHeapBudgets(s_Data->Allocator, budgets.data());

		ZN_CORE_WARN("-----------------------------------");
		for (VmaBudget& b : budgets)
		{
			auto& s = b.statistics;

			ZN_CORE_WARN("VmaBudget.allocationBytes = {0}", Utils::BytesToString(s.allocationBytes));
			ZN_CORE_WARN("VmaBudget.blockBytes = {0}", Utils::BytesToString(s.blockBytes));

			ZN_CORE_WARN("VmaBudget.usage = {0}", Utils::BytesToString(b.usage));
			ZN_CORE_WARN("VmaBudget.budget = {0}", Utils::BytesToString(b.budget));
		}
		ZN_CORE_WARN("-----------------------------------");
	}

	GPUMemoryStats VulkanAllocator::GetStats()
	{
		const auto& memoryProps = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetMemoryProperties();
		std::vector<VmaBudget> budgets(memoryProps.memoryHeapCount);

		vmaGetHeapBudgets(s_Data->Allocator, budgets.data());

		uint64_t budget = 0;
		for (VmaBudget& b : budgets)
			budget += b.budget;

		GPUMemoryStats result;
		for (const auto& [k, v] : s_AllocationMap)
		{
			if (v.Type == AllocationType::Buffer)
			{
				result.BufferAllocationCount++;
				result.BufferAllocationSize += v.AllocatedSize;
			}
			else if (v.Type == AllocationType::Image)
			{
				result.ImageAllocationCount++;
				result.ImageAllocationSize += v.AllocatedSize;
			}
		}

		result.AllocationCount = s_AllocationMap.size();
		result.Used = s_Data->MemoryUsage;
		result.TotalAvailable = budget;
		return result;
	}

	void VulkanAllocator::Init(Ref<VulkanDevice> device)
	{
		s_Data = znew VulkanAllocatorData();

		// Initialize VulkanMemoryAllocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorInfo.physicalDevice = device->GetPhysicalDevice()->GetVulkanPhysicalDevice();
		allocatorInfo.device = device->GetVulkanDevice();
		allocatorInfo.instance = VulkanContext::GetInstance();

		vmaCreateAllocator(&allocatorInfo, &s_Data->Allocator);
	}

	void VulkanAllocator::Shutdown()
	{
		vmaDestroyAllocator(s_Data->Allocator);

		delete s_Data;
		s_Data = nullptr;
	}

	VmaAllocator& VulkanAllocator::GetVMAAllocator()
	{
		return s_Data->Allocator;
	}

} 
