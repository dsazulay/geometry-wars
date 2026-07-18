#include "base/types.h"
#include "base/logger.h"
#include "vulkan_engine.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>


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
    vulkanEngine.init(instanceExtensionsCount,instanceExtensions);

    SDL_Window* window = SDL_CreateWindow("How to Vulkan", 1280u, 720u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    chk(window != nullptr);

    VkSurfaceKHR surface{ VK_NULL_HANDLE };
    chk(SDL_Vulkan_CreateSurface(window, vulkanEngine.instance(), nullptr, &surface));
    glm::ivec2 windowSize;
    chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));
    vulkanEngine.setSurfaceAndWindowSize(surface, windowSize.x, windowSize.y);
    vulkanEngine.createSwapchain();
    vulkanEngine.createSyncObjects();

    bool quit = false;
    while (!quit)
    {
        for (SDL_Event event; SDL_PollEvent(&event);)
        {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
                break;
            }
        }

        vulkanEngine.render();
    }

    vulkanEngine.terminate();
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();

    return 0;
}
