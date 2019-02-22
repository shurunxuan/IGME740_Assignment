struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer ScaleData : register(b0)
{
    float2 scale; // The render target width/height.
};

Texture2D renderTexture : register(t0);
SamplerState pointSampler : register(s0);
SamplerState linearSampler : register(s1);

float4 main(VertexToPixel input) : SV_TARGET
{
    if (scale.x == 1.0f && scale.y == 1.0f)
        return saturate(renderTexture.Sample(pointSampler, input.uv));
    else
        return saturate(renderTexture.Sample(linearSampler, input.uv / scale));
}