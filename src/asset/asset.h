#pragma once

#include "../base/types.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <slang/slang-com-ptr.h>
#include <slang/slang.h>

#include <string>
#include <vector>
#include <filesystem>


struct Vertex
{
    auto operator==(const Vertex& other) const -> bool
    {
        return pos == other.pos && texCoord == other.texCoord;
    }

    glm::vec3 pos;
    glm::vec2 texCoord;
};

namespace std
{
    template<> struct hash<Vertex>
    {
        auto operator()(Vertex const& vertex) const -> size_t
        {
            return hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct Model 
{
    std::vector<Vertex> vertices;
    std::vector<u16> indices;
};

struct Texture
{
    auto setTextureData(u64 size, const u8* data) -> void
    {
        dataSize = size;
        pData.assign(data, data + size);
    }

    u32 width;
    u32 height;
    u32 depth;
    u32 mipLevels;
    std::vector<u64> mipOffset;
    u64 dataSize;
    std::vector<u8> pData;
};

struct Shader
{
    std::string filePath;
    std::filesystem::file_time_type lastWriteTime;
    bool reloaded;

    Slang::ComPtr<ISlangBlob> spirv;
    u64 bufferSize;
    u32* bufferPointer;
};
