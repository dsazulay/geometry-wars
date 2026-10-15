#include "base/random.h"
#include "base/types.h"
#include "base/logger.h"
#include "vulkan_engine.h"
#include "components.h"
#include "asset/resource_manager.h"
#include "game.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

constexpr f32 WIDTH = 1280.0;
constexpr f32 HEIGHT = 720.0;

constexpr glm::vec3 BG_POS{ 0.0f, 0.0f, -0.1f };
constexpr glm::vec2 BG_SCALE{ 700, 700 };

struct BackgroundUniform
{
    glm::mat4 projection;
    glm::mat4 model;
};

struct EntityUniform
{
    glm::mat4 projection;
    glm::mat4 model[100];
    u32 tex[100];
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
    Random::init();

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

    Model* quad = ResourceManager::loadModel(NativeModel::Quad, "Quad");
    Shader* texShader = ResourceManager::loadShader("resources/tex_shader.slang", "Texture");
    Shader* bgShader = ResourceManager::loadShader("resources/bg_shader.slang", "Bg");
    Texture* playerTex = ResourceManager::loadTexture("resources/player.ktx", "Player");
    Texture* projectileTex = ResourceManager::loadTexture("resources/projectile.ktx", "Projectile");
    Texture* squareTex = ResourceManager::loadTexture("resources/squaresquare.ktx", "Square");
    Texture* circleTex = ResourceManager::loadTexture("resources/circle.ktx", "Circle");
    Texture* diamondTex = ResourceManager::loadTexture("resources/diamond.ktx", "Diamond");

    vulkanEngine.loadTextureData(*playerTex);
    vulkanEngine.loadTextureData(*projectileTex);
    vulkanEngine.loadTextureData(*squareTex);
    vulkanEngine.loadTextureData(*circleTex);
    vulkanEngine.loadTextureData(*diamondTex);

    vulkanEngine.createTextureDescriptors();

    MeshID quadID = vulkanEngine.loadMeshData(quad->vertices, quad->indices);

    ShaderID texShaderID = vulkanEngine.loadShader(texShader->bufferSize, texShader->bufferPointer);
    PipelineID entityPipelineID = vulkanEngine.createPipeline(texShaderID, Blending::ALPHA_BLEND);

    ShaderID bgShaderID = vulkanEngine.loadShader(bgShader->bufferSize, bgShader->bufferPointer);
    PipelineID bgPipelineID = vulkanEngine.createPipeline(bgShaderID);

    glm::mat4 proj = glm::ortho(-(WIDTH * 0.5f), WIDTH * 0.5f, -(HEIGHT * 0.5f), HEIGHT * 0.5f, -1.0f, 1.0f);

    GameObjectID bgGO = vulkanEngine.addGameObject(quadID, bgPipelineID);
    Transform bgTransform;
    bgTransform.scale(BG_SCALE);
    bgTransform.pos(BG_POS);

    Transform bulletTransfrom;
    bulletTransfrom.scale({ 20, 20 });
    bulletTransfrom.pos(BG_POS);

    BackgroundUniform bgUniform{ proj, bgTransform.model() };
    vulkanEngine.setUniformData(bgGO, &bgUniform, sizeof(BackgroundUniform));

    Game game;
    game.gameWidth = 700;
    game.gameHeight= 700;
    game.init();

    GameObjectID entityGO = vulkanEngine.addGameObject(quadID, entityPipelineID);
    EntityUniform entityUniform{ proj, {}, {} };
    vulkanEngine.setUniformData(entityGO, &entityUniform, sizeof(EntityUniform));
    vulkanEngine.createUniformBuffers();

    bool quit = false;
    bool isMouseOnScreen = false;
    u64 previousFrameTime = SDL_GetTicks();
    while (!quit)
    {
        u64 currentFrameTime = SDL_GetTicks();
        f32 deltaTime = (currentFrameTime - previousFrameTime) / 1000.0f;

        // TODO: should move this?
        vulkanEngine.render();

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
            if (event.type == SDL_EVENT_WINDOW_MOUSE_ENTER)
            {
                isMouseOnScreen = true;
            }
            if (event.type == SDL_EVENT_WINDOW_MOUSE_LEAVE)
            {
                isMouseOnScreen = false;
            }

        }

        const bool* state = SDL_GetKeyboardState(NULL);
        glm::vec2 inputDir{ 0.0f };
        if (state[SDL_SCANCODE_W]) inputDir.y += -1.0f;
        if (state[SDL_SCANCODE_S]) inputDir.y +=  1.0f;
        if (state[SDL_SCANCODE_A]) inputDir.x += -1.0f;
        if (state[SDL_SCANCODE_D]) inputDir.x +=  1.0f;

        f32 mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        game.update(deltaTime, isMouseOnScreen, { mouseX, mouseY }, inputDir);

        // NOTE: Copy relevant information from game
        u64 entityCount = 0;
        entityUniform.model[entityCount] = game.player.transform.model();
        entityUniform.tex[entityCount] = 0;
        ++entityCount;

        for (auto& p : game.projectiles)
        {
            entityUniform.model[entityCount] = p.transform.model();
            entityUniform.tex[entityCount] = 1;
            ++entityCount;
        }

        for (auto& e : game.enemies)
        {
            entityUniform.model[entityCount] = e.transform.model();
            entityUniform.tex[entityCount] = 2 + (i32) e.type;
            ++entityCount;
        }

        vulkanEngine.updateGameObjectInstanceCount(entityGO, entityCount);

        vulkanEngine.update();

        previousFrameTime = currentFrameTime;
    }

    vulkanEngine.terminate();
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();

    return 0;
}
