#pragma once
#include "GLOBALS.h"

#if VULKAN == 1
#include "vulkan/Vulkan.h"
#include "VulkanBuffer.h"
#include "Backends.h"
#include "BoronGuiTypes.h"
#include "BoronMathLibrary.h"

class BoronGui_implVulkan : public BoronGuiBackends::Backends {
public:
    static void BeginFrame();
    static void SetupRenderState(VkCommandBuffer commandBuffer);
    static void EndFrame();
    static const BoronGuiNeeds& GetGuiNeeds();

    void Init() override;
    void ReSizeViewport(GPUVector2 p_newSize) override;
    void SetBoronGuiNeeds(BoronGuiNeeds& p_boronGuiNeeds) override;
    void UpdatePerFrameOBJ(PerFrameStuct& p_perFrameStuct) override;
    void RenderAFrame(Borongui::Frame frame) override;

    void UploadBatch(const std::vector<GuiVertex>& vertices, const std::vector<uint32_t>& p_indices) override;
    void DrawBatch() override;

    static bool InitPipeline();
private:
    struct CommonPushConstant {
        GPUVector4 color{};
        GPUVector2 pos{};
        GPUVector2 size{};
    };

    struct GlobalPushConstant {
        GPUVector2 viewportSize{};
    };

    struct GuiPropertiesPushConstant {
        float rounding = 10.0f;
    };

    static GlobalPushConstant m_globalPushConstant;
    static VkShaderModule m_vertShaderModule;
    static VkShaderModule m_fragShaderModule;
    static BoronGuiNeeds m_boronGuiNeeds;
    static VkPipelineLayout m_pipelineLayout;
    static VkPipeline m_graphicsPipeline;
    static VulkanBuffer m_vkBuffer; // This is just for test
    static VulkanBuffer m_vkBufferIndex; // This is just for test

    static VkIndexType indexType;

    static VkCommandBuffer m_commandBuffer;
};
#endif