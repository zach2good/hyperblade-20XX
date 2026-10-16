// SDL wants vertex uniform buffers in space1 (set 1 on Vulkan).
cbuffer Uniforms : register(b0, space1)
{
    float4x4 transform;
};

struct Input
{
    float3 position : TEXCOORD0;
    float4 colour : TEXCOORD1;
};

struct Output
{
    float4 colour : TEXCOORD0;
    float4 position : SV_Position;
};

Output main(Input input)
{
    Output output;
    output.colour = input.colour;
    output.position = mul(transform, float4(input.position, 1.0));
    return output;
}
