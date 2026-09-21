#include "resource_manager.h"

#include <slang/slang.h>
#include <ktx.h>
#include <ktxvulkan.h>

#include <filesystem>

std::unordered_map<std::string, Shader> ResourceManager::shaders;
std::unordered_map<std::string, Texture> ResourceManager::textures;
std::unordered_map<std::string, Model> ResourceManager::models;

Slang::ComPtr<slang::IGlobalSession> ResourceManager::m_slangGlobalSession;
std::vector<slang::TargetDesc> ResourceManager::m_targets;
std::vector<slang::CompilerOptionEntry> ResourceManager::m_options;
slang::SessionDesc ResourceManager::m_slangSessionDesc;
bool ResourceManager::initialized = false;

auto ResourceManager::initShaderCompiler() -> void
{
    // Initialize Slang shader compiler
    slang::createGlobalSession(m_slangGlobalSession.writeRef());
    m_targets = {{
        .format = SLANG_SPIRV,
        .profile = m_slangGlobalSession->findProfile("spirv_1_4")
    }};
    m_options = {{
        slang::CompilerOptionName::EmitSpirvDirectly,
        { slang::CompilerOptionValueKind::Int, 1 }
    }};
    m_slangSessionDesc = slang::SessionDesc{
        .targets = m_targets.data(),
        .targetCount = SlangInt(m_targets.size()),
        .defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
        .compilerOptionEntries = m_options.data(),
        .compilerOptionEntryCount = uint32_t(m_options.size())
    };
}

auto ResourceManager::loadShader(const char* shaderFile, std::string name) -> Shader*
{
    if (!initialized)
    {
        initShaderCompiler();
        initialized = true;
        //LOG_INFO("Init slang session");
    }

    Slang::ComPtr<slang::ISession> session;
    m_slangGlobalSession->createSession(m_slangSessionDesc, session.writeRef());

    // Load shader
    Slang::ComPtr<ISlangBlob> diagnostics;
    Slang::ComPtr<slang::IModule> slangModule{
        session->loadModuleFromSource(shaderFile, shaderFile, nullptr, diagnostics.writeRef())
    };

    if (diagnostics)
    {
        //LOG_ERROR("Slang diagnostics: {}", (char*)diagnostics->getBufferPointer());
    }

    Slang::ComPtr<ISlangBlob> spirv;
    slangModule->getTargetCode(0, spirv.writeRef());

    Shader& shader = shaders[name];
    shader.filePath = shaderFile;
    shader.lastWriteTime = std::filesystem::last_write_time(shaderFile);
    shader.reloaded = false;
    shader.spirv = spirv;
    shader.bufferSize = spirv->getBufferSize();
    shader.bufferPointer = (uint32_t*) spirv->getBufferPointer();

    return &shader;
}

auto ResourceManager::loadTexture(const char* textureFile, std::string name) -> Texture*
{
    ktxTexture* ktxTex{ nullptr };
    ktxTexture_CreateFromNamedFile(textureFile, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);

    Texture& texture = textures[name];
    texture.width = ktxTex->baseWidth;
    texture.height = ktxTex->baseHeight;
    texture.depth = ktxTex->baseDepth;
    texture.mipLevels = ktxTex->numLevels;

    for (u32 j = 0; j < ktxTex->numLevels; ++j)
    {
        ktx_size_t mipOffset{0};
        KTX_error_code ret = ktxTexture_GetImageOffset(ktxTex, j, 0, 0, &mipOffset);
        if (ret != KTX_SUCCESS)
        {
            //LOG_ERROR("Error when querying mip offset");
        }
        texture.mipOffset.push_back(mipOffset);
    }

    texture.setTextureData(ktxTex->dataSize, ktxTex->pData);

    ktxTexture_Destroy(ktxTex);
    return &textures[name];
}

auto ResourceManager::loadModel(NativeModel type, std::string name) -> Model*
{
    Model model;

    model.vertices.reserve(m_vertices.size());
    model.indices.reserve(m_indices.size());

    for (int i = 0; i < static_cast<int>(m_vertices.size()); i += 4)
    {
        Vertex vertex{};
        vertex.pos = {
            m_vertices[i], m_vertices[i + 1], 0.0f
        };

        vertex.texCoord = {
            m_vertices[i + 2], m_vertices[i + 3]
        };

        model.vertices.emplace_back(vertex);
    }

    for (auto& index : m_indices)
    {
        model.indices.emplace_back(index);
    }

    models[name] = model;
    Model* ptr = &models[name];

    return ptr;
}
