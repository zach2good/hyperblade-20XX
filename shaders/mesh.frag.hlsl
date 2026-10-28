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
    float4 Shadows;    // x is one when the shadow maps are drawn; y is one for Phong shading; z is the edge's spread.
    float4 WorldLightDirs[3];
    row_major float4x4 LightClipFromWorld[2];
    float4 ArenaEllipsoid; // The collision ellipsoid's semi-axes.
    float4 Palette[256];
};

// The drome's faces are flat chords across the collision ellipsoid, up to two units inside it
// where they are widest, at the bottom. The dromers stand on the ellipsoid and are drawn over the
// drome, so a foot below a face shows where the eye's line through the face meets the ellipsoid.
// The floor reads its shadows there too. Faces well inside the ellipsoid are other structures.
static const float ShellDepth = 0.95;

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
    nointerpolation float3 Strengths : TEXCOORD9;
    float4 View : TEXCOORD10;
    float4 Pixel : SV_Position;
};

// The disc of taps that spreads a shadow's edge, Shadows.z world units across.
static const float2 SoftTaps[16] = {
    float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725), float2(-0.09418410, -0.92938870), float2(0.34495938, 0.29387760),
    float2(-0.91588581, 0.45771432), float2(-0.81544232, -0.87912464), float2(-0.38277543, 0.27676845), float2(0.97484398, 0.75648379),
    float2(0.44323325, -0.97511554), float2(0.53742981, -0.47373420), float2(-0.26496911, -0.41893023), float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507), float2(-0.81409955, 0.91437590), float2(0.19984126, 0.78641367), float2(0.14383161, -0.14100790),
};

// Where the eye's line through a point inside the ellipsoid leaves it, or along the radius when
// there is no eye.
float3 OnShell(float3 position, float4 view, float shell)
{
    if (view.w == 0.0)
    {
        return position / shell;
    }

    const float3 p = position / ArenaEllipsoid.xyz;
    const float3 d = normalize(view.xyz) / ArenaEllipsoid.xyz;

    const float a = dot(d, d);
    const float b = dot(p, d);
    const float c = dot(p, p) - 1.0;

    return position + normalize(view.xyz) * ((-b + sqrt(b * b - a * c)) / a);
}

// How much of the light reaches the point, from nought where something nearer the light hides all
// of the disc around it; outside the map it reaches. The map's depth ends at the furthest caster,
// so a point past it is behind them all and compares at the far plane, where an empty texel still
// reads as lit. The disc turns from pixel to pixel, which trades banding for fine noise.
float Lit(Texture2D<float> map, SamplerComparisonState comparison, float4 position, row_major float4x4 lightClipFromWorld, float2 pixel)
{
    const float2 uv = position.xy * float2(0.5, -0.5) + 0.5;

    if (any(uv < 0.0) || any(uv > 1.0))
    {
        return 1.0;
    }

    const float depth = min(position.z, 1.0);
    const float2 radius = Shadows.z * 0.5 * float2(length(lightClipFromWorld[0].xyz), length(lightClipFromWorld[1].xyz));

    const float angle = 6.2831853 * frac(52.9829189 * frac(dot(pixel, float2(0.06711056, 0.00583715))));
    const float2x2 turn = float2x2(cos(angle), -sin(angle), sin(angle), cos(angle));

    float lit = 0.0;

    for (int i = 0; i < 16; i++)
    {
        lit += map.SampleCmpLevelZero(comparison, uv + mul(turn, SoftTaps[i]) * radius, depth);
    }

    return lit / 16.0;
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

    // Phong: each light's term again at the pixel, from the blended normal, in place of the corners'.
    if (Shadows.y != 0.0)
    {
        const float3 normal = normalize(input.WorldNormal);
        const float3 strengths = input.Strengths;

        float pixelTerms[3];

        for (int i = 0; i < 3; i++)
        {
            const float term = dot(WorldLightDirs[i].xyz, normal) * strengths[i];

            pixelTerms[i] = term;

            if ((float)i != input.Mode.z)
            {
                pixelTerms[i] = max(term, 0.0);
            }
        }

        terms = float3(pixelTerms[0], pixelTerms[1], pixelTerms[2]);
    }

    // A shadow takes away only light that would have reached the face.
    if (Shadows.x != 0.0)
    {
        float4 shadow0 = input.ShadowPosition0;
        float4 shadow1 = input.ShadowPosition1;

        const float shell = length(input.WorldPosition / ArenaEllipsoid.xyz);

        if (input.Mode.y != 0.0 && shell > ShellDepth && shell < 1.0)
        {
            const float4 onShell = float4(OnShell(input.WorldPosition, input.View, shell), 1.0);

            shadow0 = mul(LightClipFromWorld[0], onShell);
            shadow1 = mul(LightClipFromWorld[1], onShell);
        }

        if (terms.x > 0.0)
        {
            terms.x *= Lit(ShadowMap0, ShadowSampler0, shadow0, LightClipFromWorld[0], input.Pixel.xy);
        }

        if (terms.y > 0.0)
        {
            terms.y *= Lit(ShadowMap1, ShadowSampler1, shadow1, LightClipFromWorld[1], input.Pixel.xy);
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
