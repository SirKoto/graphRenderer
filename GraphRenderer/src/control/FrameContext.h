#pragma once
#include "GlobalContext.h"

#include "../graphics/RenderSubmitter.h"

namespace gr
{

class FrameContext
{
public:

	static std::vector<FrameContext> createContexts(
		uint32_t num, GlobalContext* globalContext);

	FrameContext() = default;
	FrameContext(FrameContext&&) = default;
	FrameContext& operator=(const FrameContext&) = delete;
	FrameContext& operator=(FrameContext&&) = default;



	vkg::RenderContext& rc() { return mGlobalContext->rc(); }
	const vkg::RenderContext& rc() const { return mGlobalContext->rc(); }
	GlobalContext& gc() { return *mGlobalContext; }
	const GlobalContext& gc() const { return *mGlobalContext; }
	vkg::Window& getWindow() { return mGlobalContext->getWindow(); }
	const vkg::Window& getWindow() const { return mGlobalContext->getWindow(); }

	// From 0 to MAX_FRAMES_IN_FLIGHT
	uint32_t getIdx() const { return mFrameId; }
	uint32_t getNumConcurrentFrames() const { return CONCURRENT_FRAMES; }
	void advanceFrameCount() { mFrameCount += CONCURRENT_FRAMES; }
	uint64_t getFrameCount() const { return mFrameCount; }
	uint64_t getNextFrameCount() const { return mFrameCount + CONCURRENT_FRAMES; }
	void setImageIdx(uint32_t idx) { mImageIdx = idx; }
	uint32_t getImageIdx() const { return mImageIdx; }

	void updateTime(double_t newTime);

	double_t time() const { return mTime; }
	double_t dt() const { return mDeltaTime; }
	float_t timef() const { return static_cast<float_t>(mTime); }
	float_t dtf() const { return static_cast<float_t>(mDeltaTime); }

	vkg::ResetCommandPool& graphicsPool() { return mPools.graphicsPool; };
	const vkg::ResetCommandPool& graphicsPool() const { return mPools.graphicsPool; };
	vkg::ResetCommandPool& presentPool() { return mPools.presentPool; };
	const vkg::ResetCommandPool& presentPool() const { return mPools.presentPool; };
	vkg::ResetCommandPool& transferPool() { return mPools.transferTransientPool; };
	const vkg::ResetCommandPool& transferPool() const { return mPools.transferTransientPool; };

	vkg::RenderSubmitter& renderSubmitter() { return mRenderSubmitter; }
	const vkg::RenderSubmitter& renderSubmitter() const { return mRenderSubmitter; }


	void scheduleToDestroy(const vkg::Buffer& buffer);
	void scheduleToDestroy(const vkg::Image2D& image);
	template<typename T>
	void scheduleToDestroy(const T& obj);


	void resetFrameResources();
	void recreateCommandPools();

	void destroy();

private:
	uint32_t CONCURRENT_FRAMES = 0;
	uint32_t mFrameId = 0;
	uint64_t mFrameCount = 0;
	uint32_t mImageIdx = 0;
	double_t mTime = 0.0;
	double_t mDeltaTime = 1/30.0;

	vkg::RenderContext::FrameCommandPools mPools;
	vkg::RenderSubmitter mRenderSubmitter;

	struct DelRes;
	std::vector<std::unique_ptr<DelRes>> mResourcesToDelete;


	GlobalContext* mGlobalContext;

	void destroyCommandPools();

	FrameContext(uint32_t numMax, uint32_t id, GlobalContext* globalContext) :
		CONCURRENT_FRAMES(numMax), mFrameId(id), mFrameCount(id), mGlobalContext(globalContext) {}


	struct DelRes {
		static constexpr size_t SIZE = 4 * sizeof(void*);
		typedef std::aligned_storage<SIZE>::type Stack;
		Stack mBuffer;
		virtual void destroy(GlobalContext* gc) { assert(false); };
	};

	struct DelResBuffer : public DelRes {
		static_assert(sizeof(vkg::Buffer) <= SIZE, "Not enough memory");
		DelResBuffer(const vkg::Buffer& buffer);
		virtual void destroy(GlobalContext* gc) override final;
	};

	struct DelResImage : public DelRes {
		static_assert(sizeof(vkg::Image2D) <= SIZE, "Not enough memory");
		DelResImage(const vkg::Image2D& image);
		virtual void destroy(GlobalContext* gc) override final;
	};

	template<typename T>
	struct DelResTemp : public DelRes {
		static_assert(sizeof(T) <= SIZE, "Not enough memory");
		DelResTemp(const T& obj) {
			std::memcpy(&mBuffer, &obj, sizeof(obj));
		}
		virtual void destroy(GlobalContext* gc) override final {
			gc->rc().destroy(reinterpret_cast<T&>(mBuffer));
		}
	};

};

template<typename T>
inline void FrameContext::scheduleToDestroy(const T& obj)
{
	if (obj) {
		mResourcesToDelete.push_back(
			std::unique_ptr<DelRes>(new DelResTemp<T>(obj)));
	}
}

}