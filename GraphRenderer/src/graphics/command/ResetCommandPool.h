#pragma once

#include <vulkan/vulkan.hpp>

namespace gr
{
namespace vkg
{
	class RenderContext;
	// Warning this command pool does not have a free implemented
	class ResetCommandPool
	{
	public:
		ResetCommandPool();
		ResetCommandPool(ResetCommandPool&& o) = default;
		ResetCommandPool(uint32_t familyIdx, vk::CommandPoolCreateFlags flags, vk::Device device);

		ResetCommandPool& operator=(const ResetCommandPool& o) = delete;
		ResetCommandPool& operator=(ResetCommandPool&& o) = default;


		// num must be different from zero
		std::vector<vk::CommandBuffer> newCommandBuffers(const uint32_t num);

		vk::CommandBuffer newCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

		void submitCommandBuffer(const vk::CommandBuffer commandBuffer,
			const vk::Semaphore* waitSemaphore,
			const vk::PipelineStageFlags waitPipelineStage,
			const vk::Semaphore* signalSemaphore,
			vk::Fence fenceToSignal = nullptr) const;

		bool submitPresentationImage(
			const vk::SwapchainKHR swapChain,
			const uint32_t imageIdx,
			const vk::Semaphore* semaphoreToWait) const;

		void reset(bool release = false);

		void destroy();

		explicit operator vk::CommandPool() const;

		vk::CommandPool get() const;

	protected:
		struct CommandSpace
		{
			vk::CommandPool pool = nullptr;
			uint32_t nextToUseP = 0;
			uint32_t nextToUseS = 0;

			std::vector<vk::CommandBuffer> buffersP;
			std::vector<vk::CommandBuffer> buffersS;

		};

		std::vector<CommandSpace> mCommandSpaces;
		vk::Device mDevice;
		vk::Queue mQueue;

		CommandSpace& getCS();
		const CommandSpace& getCS() const;

	};

}; // namespace vkg
}; // namespace gr

