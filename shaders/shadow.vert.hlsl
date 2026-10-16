// Draws a mesh into a shadow map, as seen from the light.

cbuffer ShadowUniforms : register(b0, space1)
{
    row_major float4x4 LightClipFromModel;
};

float4 main(float3 position : TEXCOORD0) : SV_Position
{
    return mul(LightClipFromModel, float4(position, 1.0));
}
