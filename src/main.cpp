#include "base/types.h"
#include "base/logger.h"
#include "vulkan_engine.h"
#include "components.h"
#include "asset/resource_manager.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

constexpr float WIDTH = 1280.0;
constexpr float HEIGHT = 720.0;

constexpr glm::vec3 BG_POS{ 640.0f, 320.0f, -0.1f };
constexpr glm::vec2 BG_SCALE{ 100, 100 };

struct BackgroundUniform
{
    glm::mat4 projection;
    glm::mat4 model;
};

static inline auto chk(bool result) -> void
{
    if (!result)
    {
        logger::logError("SDL call returned an error: {}", SDL_GetError());
        exit(result);
    }
}

auto main() -> i32
{
    chk(SDL_Init(SDL_INIT_VIDEO));
    chk(SDL_Vulkan_LoadLibrary(NULL));

    u32 instanceExtensionsCount = 0;
    const char* const* instanceExtensions = SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount);

    VulkanEngine vulkanEngine;
    vulkanEngine.init(instanceExtensionsCount, instanceExtensions);

    SDL_Window* window = SDL_CreateWindow("How to Vulkan", 1280u, 720u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    chk(window != nullptr);

    VkSurfaceKHR surface{ VK_NULL_HANDLE };
    chk(SDL_Vulkan_CreateSurface(window, vulkanEngine.instance(), nullptr, &surface));
    glm::ivec2 windowSize;
    chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));
    vulkanEngine.setSurfaceAndWindowSize(surface, windowSize.x, windowSize.y);
    vulkanEngine.createSwapchain();
    vulkanEngine.createSyncObjects();

    Model* quad = ResourceManager::loadModel(NativeModel::Quad, "QuadModel");
    Shader* texShader = ResourceManager::loadShader("resources/tex_shader.slang", "TextureShader");
    Texture* spriteAtlas = ResourceManager::loadTexture("resources/player.ktx", "CardTexture");

    vulkanEngine.loadTextureData(*spriteAtlas);

    MeshID quadID = vulkanEngine.loadMeshData(quad->vertices, quad->indices);

    ShaderID texShaderID = vulkanEngine.loadShader(texShader->bufferSize, texShader->bufferPointer);
    PipelineID objPipelineID = vulkanEngine.createPipeline(texShaderID, Blending::ALPHA_BLEND);

    GameObjectID bgGO = vulkanEngine.addGameObject(quadID, objPipelineID);

    glm::mat4 proj = glm::ortho(0.0f, WIDTH, 0.0f, HEIGHT, -1.0f, 1.0f);
    Transform bgTransform;
    bgTransform.pos(BG_POS);
    bgTransform.scale(BG_SCALE);

    BackgroundUniform bgUniform{ proj, bgTransform.model() };
    vulkanEngine.setUniformData(bgGO, &bgUniform, sizeof(BackgroundUniform));
    vulkanEngine.createUniformBuffers();

    bool quit = false;
    while (!quit)
    {
        for (SDL_Event event; SDL_PollEvent(&event);)
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                quit = true;
                break;
            }
            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));
                vulkanEngine.windowSize = windowSize;
                vulkanEngine.updateSwapchain = true;
			}
        }

        vulkanEngine.render();
        vulkanEngine.update();
    }

    vulkanEngine.terminate();
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();

    return 0;
}
