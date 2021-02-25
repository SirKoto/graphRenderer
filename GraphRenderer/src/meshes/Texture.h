#pragma once

#include "../graphics/resources/Image2D.h"
#include "IObject.h"

namespace gr
{
// Forward declaration
namespace vkg {
class RenderContext;
}
class FrameContext;
class GlobalContext;

class Texture : public IObject
{
public:

	bool load(vkg::RenderContext* rc,
		const char* filePath);

	void scheduleDestroy(FrameContext* fc) override final;
	void destroy(GlobalContext* gc) override final;
	void renderImGui(FrameContext* fc) override final;


protected:

	vkg::Image2D mImage2d;
	std::string mPath;
};

} // namespace gr
