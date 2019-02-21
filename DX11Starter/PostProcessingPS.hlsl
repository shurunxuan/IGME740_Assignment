struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D renderTexture : register(t0);
SamplerState basicSampler : register(s0);
float4 main(VertexToPixel input) : SV_TARGET
{
    float dis = distance(input.uv, float2(0.5f, 0.5f)) / 0.707f;
    dis = dis * dis;
    return renderTexture.Sample(basicSampler, input.uv) * float4(1 - dis, 1 - dis, 1 - dis, 1);
}