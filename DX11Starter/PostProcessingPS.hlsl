struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D renderTexture : register(t0);
SamplerState pointSampler : register(s0);
SamplerState linearSampler : register(s1);

float4 main(VertexToPixel input) : SV_TARGET
{
    float dis = distance(input.uv, float2(0.5f, 0.5f)) / 0.707f;
    dis = dis * dis;
    dis = dis * 0.8;

    return saturate(renderTexture.Sample(pointSampler, input.uv) * float4(1 - dis, 1 - dis, 1 - dis, 1));
}