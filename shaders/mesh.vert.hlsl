// Projects each corner with the model's clip matrix and lights its face flat, as D3DRend::DrawFace
// lit its faces.

cbuffer MeshUniforms : register(b0, space1)
{
    row_major float4x4 ClipFromModel;
    float4 LightDir; // w is one in the drome pass.
    float4 Flags;    // x is one when the colour is a palette index.
};

cbuffer ColourUniforms : register(b1, space1)
{
    int LargeRampFirst;
    int LargeRampLast;
    int SmallRampLength;
    int LargeRampLength;
    float4 Palette[256];
};

// ColorPalette::LookupColorIntensity: the index walks its ramp by the light level, on a curve in
// the drome pass, and stays at the ramp's foot when the face is unlit.
float3 PaletteColour(int colour, float level)
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
        if (LightDir.w != 0.0)
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
    float4 Colour : TEXCOORD0;
    float2 UV : TEXCOORD1;
    float Grey : TEXCOORD2; // The textured face's colour, the material ignored.
    float4 Position : SV_Position;
};

Output main(Input input)
{
    Output output;
    output.Position = mul(ClipFromModel, float4(input.Position, 1.0));
    output.UV = input.UV;

    const float level = (LightDir.x * input.Normal.x + (LightDir.y * input.Normal.y + LightDir.z * input.Normal.z)) * input.Diffuse + input.Ambient;

    // D3DRend::DrawFace's textured grey: black at or below zero, white at or above one, else the
    // level in 255ths, truncated.
    float grey = 1.0;

    if (level <= 0.0)
    {
        grey = 0.0;
    }
    else if (level < 1.0)
    {
        grey = floor(255.0 * level) / 255.0;
    }

    output.Grey = grey;

    if (Flags.x != 0.0)
    {
        output.Colour = float4(PaletteColour((int)input.Colour.x, level), 1.0);

        return output;
    }

    const float3 rgb = float3(input.Colour.zyx);

    // MeshMaterial::CalcLightIntensity: black at or below zero, the plain material at or above
    // one, else each channel scaled and truncated.
    float3 lit = rgb;

    if (level <= 0.0)
    {
        lit = float3(0.0, 0.0, 0.0);
    }
    else if (level < 1.0)
    {
        lit = floor(rgb * level);
    }

    output.Colour = float4(lit / 255.0, 1.0);

    return output;
}
