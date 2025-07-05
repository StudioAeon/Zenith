#pragma once

#include "Zenith/Core/Base.hpp"
#include "Zenith/Renderer/Shader.hpp"
#include "Zenith/Renderer/Framebuffer.hpp"
#include "Zenith/Renderer/VertexBuffer.hpp"

#include <map>

typedef struct VkPipeline_T* VkPipeline;
typedef struct VkPipelineLayout_T* VkPipelineLayout;
typedef struct VkPipelineCache_T* VkPipelineCache;

namespace Zenith {

    enum class PrimitiveTopology
    {
        None = 0,
        Points,
        Lines,
        LineStrip,
        Triangles,
        TriangleStrip,
        TriangleFan
    };

    enum class DepthCompareOperator
    {
        None = 0,
        Never,
        NotEqual,
        Less,
        LessOrEqual,
        Greater,
        GreaterOrEqual,
        Equal,
        Always
    };

    struct PipelineSpecification
    {
        Ref<Shader> Shader;
        VertexBufferLayout Layout;
        VertexBufferLayout InstanceLayout;
        VertexBufferLayout BoneInfluenceLayout;
        Ref<Framebuffer> TargetFramebuffer;

        PrimitiveTopology Topology = PrimitiveTopology::Triangles;
        DepthCompareOperator DepthOperator = DepthCompareOperator::GreaterOrEqual;
        bool BackfaceCulling = true;
        bool DepthTest = true;
        bool DepthWrite = true;
        bool Wireframe = false;
        float LineWidth = 1.0f;

        FramebufferBlendMode BlendMode = FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha;

        std::string DebugName;
    };

    // *** CRITICAL: Add the PipelineStatistics struct that RenderCommandBuffer needs ***
    struct PipelineStatistics
    {
        uint64_t InputAssemblyVertices = 0;
        uint64_t InputAssemblyPrimitives = 0;
        uint64_t VertexShaderInvocations = 0;
        uint64_t ClippingInvocations = 0;
        uint64_t ClippingPrimitives = 0;
        uint64_t FragmentShaderInvocations = 0;
        uint64_t ComputeShaderInvocations = 0;
    };

    // Note: ResourceAccessFlags is designed to be bit-compatible and nearly identical to Vulkan's VkAccessFlagBits
    // Note: this is a bitfield
    enum class ResourceAccessFlags
    {
        None = 0,
        IndirectCommandRead = 0x00000001,
        IndexRead = 0x00000002,
        VertexAttributeRead = 0x00000004,
        UniformRead = 0x00000008,
        InputAttachmentRead = 0x00000010,
        ShaderRead = 0x00000020,
        ShaderWrite = 0x00000040,
        ColorAttachmentRead = 0x00000080,
        ColorAttachmentWrite = 0x00000100,
        DepthStencilAttachmentRead = 0x00000200,
        DepthStencilAttachmentWrite = 0x00000400,
        TransferRead = 0x00000800,
        TransferWrite = 0x00001000,
        HostRead = 0x00002000,
        HostWrite = 0x00004000,
        MemoryRead = 0x00008000,
        MemoryWrite = 0x00010000,
    };

    // *** PROMOTED CLASS: This was VulkanPipeline, now it's the main Pipeline class ***
    class Pipeline : public RefCounted
    {
    public:
        Pipeline(const PipelineSpecification& spec);
        ~Pipeline();

        PipelineSpecification& GetSpecification() { return m_Specification; }
        const PipelineSpecification& GetSpecification() const { return m_Specification; }

        void Invalidate();

        Ref<Shader> GetShader() const { return m_Specification.Shader; }

        bool IsDynamicLineWidth() const;

        // *** DIRECT VULKAN ACCESS: No more .As<VulkanPipeline>() needed ***
        VkPipeline GetVulkanPipeline() { return m_VulkanPipeline; }
        VkPipelineLayout GetVulkanPipelineLayout() { return m_PipelineLayout; }

        static Ref<Pipeline> Create(const PipelineSpecification& spec);

    private:
        PipelineSpecification m_Specification;

        VkPipelineLayout m_PipelineLayout = nullptr;
        VkPipeline m_VulkanPipeline = nullptr;
        VkPipelineCache m_PipelineCache = nullptr;
    };

}