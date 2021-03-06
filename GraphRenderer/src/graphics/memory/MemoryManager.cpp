#include "MemoryManager.h"

namespace gr
{
namespace vkg
{

MemoryManager::MemoryManager(vk::Instance instance, vk::PhysicalDevice physicalDevice, vk::Device logicalDevice)
{
	VmaAllocatorCreateInfo createInfo = {};
	createInfo.instance = instance;
	createInfo.physicalDevice = physicalDevice;
	createInfo.device = logicalDevice;
	// TODO: lost allocations

	vmaCreateAllocator(&createInfo, &mAllocator);
}


void MemoryManager::createImageAllocation(const vk::ImageCreateInfo& imageInfo,
	vk::MemoryPropertyFlags requiredProperties,
	vk::MemoryPropertyFlags preferredProperties,
	vk::Image* outImage,
	VmaAllocation* allocation,
	VmaAllocationInfo* outAllocInfo) const
{

	VmaAllocationCreateInfo createInfo = {};
	createInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(requiredProperties);
	createInfo.preferredFlags = static_cast<VkMemoryPropertyFlags>(preferredProperties);

	VkResult res = vmaCreateImage(mAllocator,
		reinterpret_cast<const VkImageCreateInfo*>(&imageInfo),
		&createInfo,
		reinterpret_cast<VkImage*>(outImage),
		allocation,
		outAllocInfo);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("Can't create image!!");
	}
}

void MemoryManager::createBufferAllocation(const vk::BufferCreateInfo& bufferInfo, 
	vk::MemoryPropertyFlags requiredProperties, 
	vk::MemoryPropertyFlags preferredProperties, 
	vk::Buffer* outBuffer,
	VmaAllocation* outAllocation,
	VmaAllocationInfo* outAllocInfo) const
{

	VmaAllocationCreateInfo createInfo = {};
	createInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(requiredProperties);
	createInfo.preferredFlags = static_cast<VkMemoryPropertyFlags>(preferredProperties);

	VkResult res = vmaCreateBuffer(mAllocator,
		reinterpret_cast<const VkBufferCreateInfo*>(&bufferInfo),
		&createInfo,
		reinterpret_cast<VkBuffer*>(outBuffer),
		outAllocation,
		outAllocInfo);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("Can't create buffer!!");
	}
}

void MemoryManager::freeAllocation(VmaAllocation allocation) const
{
	vmaFreeMemory(mAllocator, allocation);
}

void MemoryManager::getAllocationInfo(VmaAllocation allocation, vk::DeviceMemory* outDeviceMemory, vk::DeviceSize* outOffset, vk::DeviceSize* outSize) const
{

	VmaAllocationInfo info;
	vmaGetAllocationInfo(mAllocator, allocation, &info);
	*outDeviceMemory = info.deviceMemory;
	*outOffset = info.offset;
	*outSize = info.size;
}

bool MemoryManager::isMemoryMappable(VmaAllocation allocation) const
{
	VmaAllocationInfo info;
	vmaGetAllocationInfo(mAllocator, allocation, &info);
	VkMemoryPropertyFlags memFlags;
	vmaGetMemoryTypeProperties(mAllocator, info.memoryType, &memFlags);
	if ((memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
	{
		return true;
	}
	return false;
}

void* MemoryManager::mapMemory(VmaAllocation allocation) const
{
	void* ptr;
	vmaMapMemory(mAllocator, allocation, &ptr);
	return ptr;
}

void MemoryManager::unmapMemory(VmaAllocation allocation) const
{
	vmaUnmapMemory(mAllocator, allocation);
}

void MemoryManager::flushAllocations(const VmaAllocation* allocations, uint32_t num) const
{
	VkResult res = vmaFlushAllocations(mAllocator, num,
		allocations, nullptr, nullptr);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("Error! Can't flush allocations!");
	}
}

void MemoryManager::destroy()
{
	vmaDestroyAllocator(mAllocator);
}


}; // namespace vkg
}; // namespace gr