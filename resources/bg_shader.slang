struct VSInput
{
    float3 pos;
    float2 uv;
};

Sampler2D textures[];

struct ShaderData
{
    float4x4 projection;
    float4x4 model;
};

struct VSOutput {
    float4 pos : SV_POSITION;
    float2 uv;
};

[shader("vertex")]
VSOutput main(VSInput input, uniform ShaderData *shaderData, uint instanceIndex : SV_VulkanInstanceID)
{
    VSOutput output;
    float4x4 modelMat = shaderData->model;
    output.uv = input.uv;
    output.pos = mul(shaderData->projection,  mul(modelMat, float4(input.pos, 1.0)));
    return output;
}

[shader("fragment")]
float4 main(VSOutput input)
{
    float grid = smoothstep(0.98, 1, frac(input.uv.x * 10));
    grid = max(smoothstep(0.98, 1, frac(input.uv.y * 10)), grid);
    float2 uv = input.uv * 2 - 1;
    float border = smoothstep(0.99, 1, abs(uv.x));
    border = max(smoothstep(0.99, 1, abs(uv.y)), border);
    grid = max(0, grid - border);
    float4 color = float4(0, border, grid, 1);
    return color;
}
