// The face's lit colour, or for a textured face its grey level times the texel, as the original's
// MODULATE blend drew.

Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

cbuffer TextureUniforms : register(b0, space3)
{
    int Textured;
    int3 Padding;
};

struct Input
{
    float4 Colour : TEXCOORD0;
    float2 UV : TEXCOORD1;
    float Grey : TEXCOORD2;
};

float4 main(Input input) : SV_Target0
{
    if (Textured != 0)
    {
        return float4(input.Grey * Texture.Sample(Sampler, input.UV).rgb, 1.0);
    }

    return input.Colour;
}
