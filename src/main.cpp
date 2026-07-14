#define VOLK_IMPLEMENTATION
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

static inline void chk(VkResult result)
{
    if (result != VK_SUCCESS)
    {
        SDL_Log("Vulkan call returned an error: (%d)\n", result);
        exit(result);
    }
}

static inline void chk(bool result)
{
    if (!result)
    {
        SDL_Log("SDL call returned an error: %s\n", SDL_GetError());
        exit(result);
    }
}

auto main() -> int
{
    chk(SDL_Init(SDL_INIT_VIDEO));
    chk(SDL_Vulkan_LoadLibrary(NULL));
    volkInitialize();

    // Instance
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "How to Vulkan",
        .apiVersion = VK_API_VERSION_1_3
    };
    uint32_t instanceExtensionsCount = 0;
    char const* const* instanceExtensions = SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount);
    VkInstanceCreateInfo instanceCI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = instanceExtensionsCount,
        .ppEnabledExtensionNames = instanceExtensions,
    };

    VkInstance instance{ VK_NULL_HANDLE };
    chk(vkCreateInstance(&instanceCI, nullptr, &instance));
    volkLoadInstance(instance);

    SDL_Window* window = SDL_CreateWindow("How to Vulkan", 1280u, 720u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    chk(window != nullptr);

    VkSurfaceKHR surface{ VK_NULL_HANDLE };
    chk(SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface));

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
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();
    vkDestroyInstance(instance, nullptr);

    return 0;
}
