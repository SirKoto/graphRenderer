#pragma once

#include "../control/FrameContext.h"

namespace gr {


class Gui
{
public:


	
	void init(GlobalContext* gc);

	void destroy(const vkg::RenderContext& rc);

	void updatePipelineState(vkg::RenderContext* rc,
		vk::RenderPass renderPass,
		uint32_t subpass);

	void updatePreFrame(FrameContext* fc);

	void render(FrameContext* fc, vk::CommandBuffer cmd);

	void addFont(const char* filename);

	void uploadFontObjects(vkg::RenderContext* rc);

protected:

	vk::Pipeline mPipeline;
	vkg::Buffer mIndexBuffer;
	vkg::Buffer mVertexBuffer;
	uint8_t* mIdxPtrMap = nullptr;
	uint8_t* mVertPtrMap = nullptr;
	vkg::Image2D mFontImage;

	vk::DescriptorSetLayout mDescriptorLayout;
	vk::DescriptorSet mDescriptorSet;
	vk::PipelineLayout mPipelineLayout;

	vk::Sampler mTexSampler;

};

} // namespace gr

