#pragma once

#include "base/types.h"
#include "base/handle.h"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <glm/glm.hpp>

#include <cstddef>
#include <vector>
#include <array>

constexpr u32 MAX_FRAMES_IN_FLIGHT{ 2 };

struct MeshTag {};
struct ShaderTag {};
struct PipelineTag {};
struct GameObjectTag {};

using MeshID = Handle<MeshTag>;
using ShaderID = Handle<ShaderTag>;
using PipelineID = Handle<PipelineTag>;
using GameObjectID = Handle<GameObjectTag>;

enum class Blending
{
    NONE,
    ALPHA_BLEND
};

struct ShaderData
{
    void* data;
    u64 size;
};

struct ShaderDataBuffer
{
    VmaAllocation allocation{ VK_NULL_HANDLE };
    VmaAllocationInfo allocationInfo{};
    VkBuffer buffer{ VK_NULL_HANDLE };
    VkDeviceAddress deviceAddress{};
};

struct TextureGPU
{
    VmaAllocation allocation{ VK_NULL_HANDLE };
    VkImage image{ VK_NULL_HANDLE };
    VkImageView view{ VK_NULL_HANDLE };
    VkSampler sampler{ VK_NULL_HANDLE };
};

struct MeshBuffer
{
    VkBuffer buffer{ VK_NULL_HANDLE };
    VmaAllocation allocation{ VK_NULL_HANDLE };
    VkDeviceSize bufferSize{};
    VkDeviceSize indexCount{};
};

struct GameObject
{
    MeshID meshID;
    PipelineID pipelineID;
    ShaderData shaderData;
    std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT> shaderDataBuffers;
    u64 instanceCount;
};

struct Pipeline
{
    VkPipelineLayout layout{ VK_NULL_HANDLE };
    VkPipeline pipeline;
};

struct VulkanPointers
{
    VkInstance instance{ VK_NULL_HANDLE };
    VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
    VkDevice device{ VK_NULL_HANDLE };
    VkQueue queue{ VK_NULL_HANDLE };
    VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
    VkFormat imageFormat;
};

class VulkanEngine
{
public:
    auto init(u32 extensionsCount, const char* const* requiredExtensions) -> void;
    auto terminate() -> void;
    auto createSwapchain() -> void;
    auto render() -> void;
    auto reloadShader(ShaderID shader, u64 bufferSize, u32* bufferPointer) -> void;
    auto reloadPipeline(PipelineID pipeline, ShaderID shader, Blending blending) -> void;

    auto setSurfaceAndWindowSize(VkSurfaceKHR surface, u32 sizeX, u32 sizeY) -> void;
    auto createImguiDescriptorPool() -> void;
    //auto loadMeshData(std::vector<Vertex>& vertices, std::vector<u16>& indices) -> MeshID;
    //auto loadTextureData(Texture& texture) -> void;
    auto loadShader(u64 bufferSize, u32* bufferPointer) -> ShaderID;
    auto setUniformData(GameObjectID id, void* data, u64 size) -> void;
    auto createPipeline(ShaderID shaderID, Blending blending = Blending::NONE) -> PipelineID;
    auto createUniformBuffers() -> void;
    auto addGameObject(MeshID id, PipelineID pipelineID) -> GameObjectID;
    auto updateGameObjectInstanceCount(GameObjectID id, u64 instanceCount) -> void;
    auto getVulkanPointers() -> VulkanPointers;
    auto instance() -> VkInstance;
    auto waitDevice() -> void;

    auto createSyncObjects() -> void;

private:
    auto setAlphaBlendAttachment() -> VkPipelineColorBlendAttachmentState;
    auto createPipelineLayout() -> VkPipelineLayout;
    auto internalCreatePipeline(VkPipelineLayout layout, ShaderID shaderID, Blending blending = Blending::NONE) -> Pipeline;
    auto internalLoadShader(u64 bufferSize, u32* bufferPointer) -> VkShaderModule;

    VkInstance m_instance{ VK_NULL_HANDLE };
    VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
    VkDevice m_device{ VK_NULL_HANDLE };
    VkQueue m_queue{ VK_NULL_HANDLE };
    VmaAllocator m_allocator{ VK_NULL_HANDLE };
    VkSurfaceKHR m_surface{ VK_NULL_HANDLE };

    VkSwapchainKHR m_swapchain{ VK_NULL_HANDLE };
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
	u32 m_imageCount;

    VkImage m_depthImage;
    VmaAllocation m_depthImageAllocation;
    VkImageView m_depthImageView;

    std::vector<MeshBuffer> m_meshBuffers;
    std::vector<GameObject> m_gameObjects;

    std::vector<VkShaderModule> m_shaderModules;
    std::vector<Pipeline> m_pipelines;

    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> m_fences;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_presentSemaphores;
    std::vector<VkSemaphore> m_renderSemaphores;

    VkCommandPool m_commandPool{ VK_NULL_HANDLE };
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_commandBuffers;

    std::array<TextureGPU, 1> m_textures{};

    VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };
    VkDescriptorSetLayout m_descriptorSetLayoutTex{ VK_NULL_HANDLE };
    VkDescriptorSet m_descriptorSetTex{ VK_NULL_HANDLE };


    u32 m_frameIndex{ 0 };
    u32 m_imageIndex{ 0 };

    glm::ivec2 m_windowSize{};

    VkDescriptorPool m_imguiPool;
};
