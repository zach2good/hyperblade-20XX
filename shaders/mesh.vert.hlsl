// Projects each corner with the model's clip matrix and hands the fragment shader what it lights
// the face with: each scene light's term, and where the corner sits in the world and in each
// shadow map.

cbuffer MeshUniforms : register(b0, space1)
{
    row_major float4x4 ClipFromModel;
    row_major float4x4 WorldFromModel;
    float4 LightDirs[3]; // In the model's space; w is the light's strength, zero for off.
    float4 Flags;        // x: the colour is a palette index. y: the key light, or -1. z: the drome's curve.
    float4 Eye;          // The camera's eye in the world; w is zero when there is none.
};

cbuffer FrameUniforms : register(b1, space1)
{
    row_major float4x4 LightClipFromWorld[2];
};

struct Input
{
    float3 Position : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 UV : TEXCOORD2;
    uint4 Colour : TEXCOORD3; // 0xAARRGGBB read byte by byte, so B, G, R, A.
    float Diffuse : TEXCOORD4;
    float Ambient : TEXCOORD5;
};

struct Output
{
    nointerpolation float4 Base : TEXCOORD0; // The palette index in x, or the RGB from 0 to 255.
    nointerpolation float4 Mode : TEXCOORD1; // x: palettised. y: the drome's curve. z: the key light.
    float2 UV : TEXCOORD2;
    float3 Terms : TEXCOORD3; // Each scene light's term, before its shadow.
    float2 Material : TEXCOORD4; // Diffuse and ambient.
    float3 WorldPosition : TEXCOORD5;
    float3 WorldNormal : TEXCOORD6;
    float4 ShadowPosition0 : TEXCOORD7;
    float4 ShadowPosition1 : TEXCOORD8;
    nointerpolation float3 Strengths : TEXCOORD9; // Each scene light's strength.
    float4 View : TEXCOORD10; // From the eye to the corner in the world; w as Eye's.
    float4 Position : SV_Position;
};

Output main(Input input)
{
    Output output;
    output.Position = mul(ClipFromModel, float4(input.Position, 1.0));
    output.UV = input.UV;

    const float3 rgb = float3(input.Colour.zyx);

    output.Base = float4(rgb, 0.0);

    if (Flags.x != 0.0)
    {
        output.Base = float4((float)input.Colour.x, 0.0, 0.0, 0.0);
    }

    output.Mode = float4(Flags.x, Flags.z, Flags.y, 0.0);
    output.Strengths = float3(LightDirs[0].w, LightDirs[1].w, LightDirs[2].w);

    // The key light is the one the original lights the face by, signed as it was; the others only
    // add where they face the surface.
    float terms[3];

    for (int i = 0; i < 3; i++)
    {
        const float term = (LightDirs[i].x * input.Normal.x + (LightDirs[i].y * input.Normal.y + LightDirs[i].z * input.Normal.z)) * LightDirs[i].w;

        terms[i] = term;

        if ((float)i != Flags.y)
        {
            terms[i] = max(term, 0.0);
        }
    }

    output.Terms = float3(terms[0], terms[1], terms[2]);
    output.Material = float2(input.Diffuse, input.Ambient);

    const float4 world = mul(WorldFromModel, float4(input.Position, 1.0));

    output.WorldPosition = world.xyz;
    output.WorldNormal = mul((float3x3)WorldFromModel, input.Normal);
    output.ShadowPosition0 = mul(LightClipFromWorld[0], world);
    output.ShadowPosition1 = mul(LightClipFromWorld[1], world);
    output.View = float4(world.xyz - Eye.xyz, Eye.w);

    return output;
}
