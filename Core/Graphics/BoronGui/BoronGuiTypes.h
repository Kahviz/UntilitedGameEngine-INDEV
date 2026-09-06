#pragma once

#include "GLOBALS.h"
#include "BoronMathLibrary.h"

#if DIRECTX11 == 1
    struct PerFrameStuct {

    };

    struct BoronGuiNeeds {

    };
#endif

struct GuiVertex {
    BML::Vec2 position;
    BML::Vec2 size;
    BML::Col255 color;
};

#if VULKAN == 1
#include "vulkan/Vulkan.h"

struct PerFrameStuct {
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
};

struct BoronGuiNeeds {
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkExtent2D swapchainExtent{};
};
#endif