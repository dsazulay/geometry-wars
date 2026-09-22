#pragma once

#include "assets.h"

#include <slang/slang-com-ptr.h>

#include <unordered_map>
#include <array>
#include <string>

enum class NativeModel
{
    Quad
};

class ResourceManager
{
public:
    static auto loadModel(NativeModel type, std::string name) -> Model*;
    static auto loadShader(const char* shaderFile, std::string name) -> Shader*;
    static auto loadTexture(const char* textureFile, std::string name) -> Texture*;

    static std::unordered_map<std::string, Model> models;
    static std::unordered_map<std::string, Shader> shaders;
    static std::unordered_map<std::string, Texture> textures;

private:
    static auto initShaderCompiler() -> void;

    static bool initialized;

    static Slang::ComPtr<slang::IGlobalSession> m_slangGlobalSession;
    static std::vector<slang::TargetDesc> m_targets;
    static std::vector<slang::CompilerOptionEntry> m_options;
    static slang::SessionDesc m_slangSessionDesc;

    static constexpr std::array<float, 16> m_vertices{
        -0.5f, -0.5f, 0.0f, 0.0f,
         0.5f, -0.5f, 1.0f, 0.0f,
         0.5f,  0.5f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 1.0f
    };

    static constexpr std::array<int, 6> m_indices{
        0, 3, 1, 3, 2, 1
    };
};
