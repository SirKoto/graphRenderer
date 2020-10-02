#include "SwapChain.h"
#include <algorithm>

namespace gr
{
namespace vkg
{

SwapChain::SwapChain(const DeviceComp& device, const Window& window)
{
	vk::SurfaceKHR surface = window.getSurface();

	// Choose Surface format
	{
		std::vector<vk::SurfaceFormatKHR> availableFormats =
			static_cast<vk::PhysicalDevice>(device).getSurfaceFormatsKHR(surface);
		bool found = false;
		for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
				availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				mFormat = availableFormat;
				found = true;
				break;
			}
		}
		if (!found) {
			mFormat = *availableFormats.begin();
		}

	}
	// Choose present mode
	vk::PresentModeKHR presentMode;
	{
		std::vector<vk::PresentModeKHR> presentModes =
			static_cast<vk::PhysicalDevice>(device).getSurfacePresentModesKHR(surface);
		if (std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eMailbox) != presentModes.end()) {
			presentMode = vk::PresentModeKHR::eMailbox;
		}
		else {
			presentMode = vk::PresentModeKHR::eFifo;
		}
	}

	// Choose number of images that the swap chain can handle, and the images extent. Also select transform
	uint32_t numImages;
	vk::SurfaceTransformFlagBitsKHR preTransform;
	{
		vk::SurfaceCapabilitiesKHR capabilities =
			static_cast<vk::PhysicalDevice>(device).getSurfaceCapabilitiesKHR(surface);

		preTransform = capabilities.currentTransform;
		// Check if the capabilites automatically assigned extent, if not then assign it
		{
			if (capabilities.currentExtent.width != UINT32_MAX) {
				mExtent = capabilities.currentExtent;
			}
			else {
				mExtent = vk::Extent2D (window.getWidth(), window.getHeigth());

				mExtent.width =
					std::max(capabilities.minImageExtent.width,
						std::min(capabilities.maxImageExtent.width,
							mExtent.width));
				mExtent.height =
					std::max(capabilities.minImageExtent.height,
						std::min(capabilities.maxImageExtent.height,
							mExtent.height));
			}
		}
		// Select minimum number of images to be able to use vSync
		numImages = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 &&
			numImages > capabilities.maxImageCount) {
			numImages = capabilities.maxImageCount;
		}
	}

	vk::SwapchainCreateInfoKHR createInfo;
	createInfo.surface = surface;
	createInfo.minImageCount = numImages;
	createInfo.imageFormat = mFormat.format;
	createInfo.imageColorSpace = mFormat.colorSpace;
	createInfo.presentMode = presentMode;
	createInfo.imageExtent = mExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	createInfo.preTransform = preTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.clipped = VK_TRUE;

	// Check if the graphics family and the present family are different
	if (device.getGraphicsFamilyIdx() != device.getPresentFamilyIdx()) {
		std::array<uint32_t, 2> indices = { device.getGraphicsFamilyIdx(),  device.getPresentFamilyIdx() };

		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = static_cast<uint32_t>(indices.size());
		createInfo.pQueueFamilyIndices = indices.data();
	}
	else {
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
	}

	mSwapChain = static_cast<vk::Device>(device).createSwapchainKHR(createInfo);

	mImages = static_cast<vk::Device>(device).getSwapchainImagesKHR(mSwapChain);
}

void SwapChain::destroy(const vk::Device& device)
{
	device.destroySwapchainKHR(mSwapChain);
}

}; // namespace vkg
}; // namespace gr