// The face's lit colour. Textured faces will modulate a texture sample by it here.

struct Input
{
    float4 Colour : TEXCOORD0;
    float2 UV : TEXCOORD1;
};

float4 main(Input input) : SV_Target0
{
    return input.Colour;
}
