struct VSInput
{
    float3 Pos;
    float2 UV;
};

Sampler2D textures[];

struct ShaderData
{
    float4x4 projection;
    float4x4 model[100];
    uint32_t tex[100];
};

struct VSOutput {
    float4 Pos : SV_POSITION;
    float2 UV;
    uint32_t tex;
};

[shader("vertex")]
VSOutput main(VSInput input, uniform ShaderData *shaderData, uint instanceIndex : SV_VulkanInstanceID)
{
    VSOutput output;
    float4x4 modelMat = shaderData->model[instanceIndex];
    output.Pos = mul(shaderData->projection,  mul(modelMat, float4(input.Pos, 1.0)));
    output.tex = shaderData->tex[instanceIndex];
    output.UV = input.UV;
    return output;
}

[shader("fragment")]
float4 main(VSOutput input)
{
    float4 color = textures[input.tex].Sample(input.UV);
    return color;
}
