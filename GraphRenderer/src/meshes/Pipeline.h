#pragma once
#include "IObject.h"

#include "ResourcesHeader.h"

namespace gr
{

class Pipeline :
    public IObject
{
public:

    Pipeline() = default;


    virtual void scheduleDestroy(FrameContext* fc) override final;

    virtual void renderImGui(FrameContext* fc, Gui* gui) override final;

    static constexpr const char* s_getClassName() { return "Pipeline"; }

private:

    vk::PipelineLayout mPipelineLayout;
    vk::Pipeline mPipeline;

    gr::ResId mMaterialDescriptorLayout;

    // Serialization functions
    template<class Archive>
    void serialize(Archive& archive)
    {
        archive(cereal::base_class<IObject>(this));
        //TODO
    }

    GR_SERIALIZE_PRIVATE_MEMBERS

}; // class Pipeline

} // namespace gr

GR_SERIALIZE_TYPE(gr::Pipeline)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::IObject, gr::Pipeline)