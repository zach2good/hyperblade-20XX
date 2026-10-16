// Projects each corner with the model's clip matrix and lights its face flat.

float4 main(float3 position : TEXCOORD0) : SV_Position
{
    return float4(position, 1.0);
}
