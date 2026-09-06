#include "BoronGui_implVulkan.h"
#include <ErrorHandling/ErrorMessage.h>

#if VULKAN == 1
#include "Shaders/Vulkan/FragmentShader.h"
#include "Shaders/Vulkan/VertexShader.h"
#include "Vulkan/VulkanHelpers.h"
#include "Vertex2d.h"
#include "Widgets/Widgets.h"

BoronGuiNeeds BoronGui_implVulkan::m_boronGuiNeeds{};
VkShaderModule BoronGui_implVulkan::m_vertShaderModule = VK_NULL_HANDLE;
VkShaderModule BoronGui_implVulkan::m_fragShaderModule = VK_NULL_HANDLE;
VkPipelineLayout BoronGui_implVulkan::m_pipelineLayout = VK_NULL_HANDLE;
VkPipeline BoronGui_implVulkan::m_graphicsPipeline = VK_NULL_HANDLE;
VulkanBuffer BoronGui_implVulkan::m_vkBuffer{}; // This is just for test
VulkanBuffer BoronGui_implVulkan::m_vkBufferIndex{}; // This is just for test
VkIndexType BoronGui_implVulkan::indexType = VK_INDEX_TYPE_UINT32;
VkCommandBuffer BoronGui_implVulkan::m_commandBuffer;
BoronGui_implVulkan::GlobalPushConstant BoronGui_implVulkan::m_globalPushConstant{};

static Vertex2d vertices[] = {
    Vertex2d(
        { -0.5f, -0.5f },
        { 1.0f, 0.0f, 0.0f }
    ),

    Vertex2d(
        { 0.5f, -0.5f },
        { 0.0f, 1.0f, 0.0f }
    ),

    Vertex2d(
        { 0.5f, 0.5f },
        { 0.0f, 0.0f, 1.0f }
    ),

    Vertex2d(
        { -0.5f, 0.5f },
        { 1.0f, 1.0f, 0.0f }
    )
};

static uint32_t indices[] = {
    0, 1, 2,
    2, 3, 0
};

void BoronGui_implVulkan::BeginFrame() {

}

void BoronGui_implVulkan::SetupRenderState(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    VkViewport viewport{};
    viewport.height = m_boronGuiNeeds.swapchainExtent.height;
    viewport.width = m_boronGuiNeeds.swapchainExtent.width;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    VkRect2D scissor{};
    scissor.extent = { m_boronGuiNeeds.swapchainExtent.width, m_boronGuiNeeds.swapchainExtent.height };

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void BoronGui_implVulkan::EndFrame() {
}

void BoronGui_implVulkan::Init() {
    m_vkBuffer.Create(
        m_boronGuiNeeds.device,
        m_boronGuiNeeds.physicalDevice,
        sizeof(vertices),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    m_vkBufferIndex.Create(
        m_boronGuiNeeds.device,
        m_boronGuiNeeds.physicalDevice,
        sizeof(indices),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    m_vkBuffer.UploadData(vertices, sizeof(vertices));
    m_vkBufferIndex.UploadData(indices, sizeof(indices));

	CreateInfo("Init func");
    InitPipeline();
}

const BoronGuiNeeds& BoronGui_implVulkan::GetGuiNeeds() {
	return m_boronGuiNeeds;
}

void BoronGui_implVulkan::ReSizeViewport(GPUVector2 p_newSize) {
    m_boronGuiNeeds.swapchainExtent.width = static_cast<uint32_t>(p_newSize.x);
    m_boronGuiNeeds.swapchainExtent.height = static_cast<uint32_t>(p_newSize.y);
}

void BoronGui_implVulkan::SetBoronGuiNeeds(BoronGuiNeeds& p_boronGuiNeeds) {
    m_boronGuiNeeds = p_boronGuiNeeds;
}

void BoronGui_implVulkan::UpdatePerFrameOBJ(PerFrameStuct& p_perFrameStuct) {
    m_commandBuffer = p_perFrameStuct.commandBuffer;
}

void BoronGui_implVulkan::RenderAFrame(Borongui::Frame frame) {
    BML::Vec2 currentSize = {
        static_cast<float>(m_boronGuiNeeds.swapchainExtent.width),
        static_cast<float>(m_boronGuiNeeds.swapchainExtent.height)
    };

    static BML::Vec2 lastSize = currentSize;

    static bool resized = true;

    if (currentSize != lastSize) {
        lastSize = currentSize;
        resized = true;

        CreateInfo("Resizing..");
    }

    VkBuffer vertexBuffers[] = {m_vkBuffer.GetBuffer()};
    VkBuffer indexBuffers[] = {m_vkBufferIndex.GetBuffer()};

    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(
        m_commandBuffer,
        0,
        1,
        vertexBuffers,
        offsets
    );

    vkCmdBindIndexBuffer(
        m_commandBuffer,
        *indexBuffers,
        0,
        indexType
    );

    CommonPushConstant commonPushConstant{};

    commonPushConstant.color = GPUVector4(frame.getColor().x() / 255.0f, frame.getColor().y() / 255.0f, frame.getColor().z() / 255.0f, 1.0f);
    commonPushConstant.pos = { frame.getPosition().x(), frame.getPosition().y() };
    commonPushConstant.size = { frame.getSize().x(), frame.getSize().y() };

    GuiPropertiesPushConstant guiPropPushConstant{};
    guiPropPushConstant.rounding = 10.0f;

    if (resized){
        m_globalPushConstant.viewportSize = { 
            static_cast<float>(m_boronGuiNeeds.swapchainExtent.width),
            static_cast<float>(m_boronGuiNeeds.swapchainExtent.height) 
        };
    }
    
    vkCmdPushConstants(
        m_commandBuffer,
        m_pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(commonPushConstant),
        &commonPushConstant
    );

    if (resized) {
        vkCmdPushConstants(
            m_commandBuffer,
            m_pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            sizeof(commonPushConstant),
            sizeof(m_globalPushConstant),
            &m_globalPushConstant
        );
    }

    vkCmdPushConstants(
        m_commandBuffer,
        m_pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        sizeof(commonPushConstant) + sizeof(m_globalPushConstant),
        sizeof(guiPropPushConstant),
        &guiPropPushConstant
    );

    vkCmdDrawIndexed(
        m_commandBuffer,
        6,
        1,
        0,
        0,
        0
    );
}

bool BoronGui_implVulkan::InitPipeline() {
    CreateInfo("Initing VulkanPipeline!");
    //if desc here it would be

    auto vertShaderCode = ReadShader(VertexShader);
    auto fragShaderCode = ReadShader(FragmentShader);

    m_vertShaderModule = CreateShaderModule(m_boronGuiNeeds.device, vertShaderCode);
    m_fragShaderModule = CreateShaderModule(m_boronGuiNeeds.device, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = m_vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = m_fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex2d::getBindingDescription();
    auto attributeDescriptions = Vertex2d::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;

    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.cullMode = VK_CULL_MODE_NONE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    //PushConstants
    VkPushConstantRange commonPushConstant = CreatePushConstantRange(
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CommonPushConstant)
    );

    VkPushConstantRange globalPushConstant = CreatePushConstantRange(
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(commonPushConstant), sizeof(GlobalPushConstant)
    );

    VkPushConstantRange guiPropertiesPushConstant = CreatePushConstantRange(
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(commonPushConstant) + sizeof(globalPushConstant), sizeof(GuiPropertiesPushConstant)
    );

    std::vector<VkPushConstantRange> pushConstants;
    pushConstants.push_back(commonPushConstant);
    pushConstants.push_back(globalPushConstant);
    pushConstants.push_back(guiPropertiesPushConstant);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();
    pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
    //

    BGE_ASSERT_VKRESULT(vkCreatePipelineLayout(m_boronGuiNeeds.device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout), "Failed to create pipeline layout!");

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_boronGuiNeeds.renderPass;
    pipelineInfo.subpass = 0;

    BGE_ASSERT_VKRESULT(vkCreateGraphicsPipelines(m_boronGuiNeeds.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline), "Failed to create graphics pipeline!");

    return true;
}
#endif