// Lights the face from the terms the vertex shader made, with each upper light shadowed and the
// rok's light added, then colours it as the original did: along the palette ramps, scaled RGB,
// or for a textured face its grey level times the texel, as the original's MODULATE blend drew.

Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

Texture2D<float> ShadowMap0 : register(t1, space2);
SamplerComparisonState ShadowSampler0 : register(s1, space2);

Texture2D<float> ShadowMap1 : register(t2, space2);
SamplerComparisonState ShadowSampler1 : register(s2, space2);

cbuffer TextureUniforms : register(b0, space3)
{
    int Textured;
    int3 Padding;
};

cbuffer ColourUniforms : register(b1, space3)
{
    int LargeRampFirst;
    int LargeRampLast;
    int SmallRampLength;
    int LargeRampLength;
    float4 PointLight; // The rok's light in the world; w is its reach, zero for none.
    float4 Shadows;    // x is one when the shadow maps are drawn.
    float4 Palette[256];
};

struct Input
{
    nointerpolation float4 Base : TEXCOORD0;
    nointerpolation float4 Mode : TEXCOORD1;
    float2 UV : TEXCOORD2;
    float3 Terms : TEXCOORD3;
    float2 Material : TEXCOORD4;
    float3 WorldPosition : TEXCOORD5;
    float3 WorldNormal : TEXCOORD6;
    float4 ShadowPosition0 : TEXCOORD7;
    float4 ShadowPosition1 : TEXCOORD8;
};

// One where the light reaches the point, zero where something nearer the light hides it; outside
// the map it reaches.
float Lit(Texture2D<float> map, SamplerComparisonState comparison, float4 position)
{
    const float2 uv = position.xy * float2(0.5, -0.5) + 0.5;

    if (any(uv < 0.0) || any(uv > 1.0) || position.z > 1.0)
    {
        return 1.0;
    }

    return map.SampleCmpLevelZero(comparison, uv, position.z);
}

// ColorPalette::LookupColorIntensity: the index walks its ramp by the light level, on a curve in
// the drome pass, and stays at the ramp's foot when the face is unlit.
float3 PaletteColour(int colour, float level, bool curve)
{
    int length = SmallRampLength;

    if (colour >= LargeRampFirst && colour <= LargeRampLast)
    {
        length = LargeRampLength;
    }

    int index = colour;

    if (level >= 1.0)
    {
        index = colour + length;
    }
    else if (level > 0.0)
    {
        if (curve)
        {
            index = (int)pow((float)(length + 1), level) + colour - 1;
        }
        else
        {
            index = (int)(length * level) + colour;
        }
    }

    return Palette[index & 255].rgb;
}

float4 main(Input input) : SV_Target0
{
    float3 terms = input.Terms;

    // A shadow takes away only light that would have reached the face.
    if (Shadows.x != 0.0)
    {
        if (terms.x > 0.0)
        {
            terms.x *= Lit(ShadowMap0, ShadowSampler0, input.ShadowPosition0);
        }

        if (terms.y > 0.0)
        {
            terms.y *= Lit(ShadowMap1, ShadowSampler1, input.ShadowPosition1);
        }
    }

    float glow = 0.0;

    if (PointLight.w > 0.0)
    {
        const float3 toLight = PointLight.xyz - input.WorldPosition;
        const float distance = length(toLight);
        const float reach = saturate(1.0 - distance / PointLight.w);

        glow = max(dot(normalize(input.WorldNormal), toLight / max(distance, 0.0001)), 0.0) * reach * reach;
    }

    const float level = (terms.x + terms.y + terms.z + glow) * input.Material.x + input.Material.y;

    if (Textured != 0)
    {
        // D3DRend::DrawFace's textured grey: black at or below zero, white at or above one, else
        // the level in 255ths, truncated.
        float grey = 1.0;

        if (level <= 0.0)
        {
            grey = 0.0;
        }
        else if (level < 1.0)
        {
            grey = floor(255.0 * level) / 255.0;
        }

        return float4(grey * Texture.Sample(Sampler, input.UV).rgb, 1.0);
    }

    if (input.Mode.x != 0.0)
    {
        return float4(PaletteColour((int)input.Base.x, level, input.Mode.y != 0.0), 1.0);
    }

    // MeshMaterial::CalcLightIntensity: black at or below zero, the plain material at or above
    // one, else each channel scaled and truncated.
    float3 lit = input.Base.rgb;

    if (level <= 0.0)
    {
        lit = float3(0.0, 0.0, 0.0);
    }
    else if (level < 1.0)
    {
        lit = floor(input.Base.rgb * level);
    }

    return float4(lit / 255.0, 1.0);
}
