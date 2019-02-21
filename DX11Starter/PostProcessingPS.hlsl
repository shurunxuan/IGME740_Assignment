struct VertexToPixel
{
    float4 position : POSITION;
    float2 uv : TEXCOORD;
};

Texture2D renderTexture : register(t0);
SamplerState basicSampler : register(s0);
float4 main(VertexToPixel input) : SV_TARGET
{
    return renderTexture.Sample(basicSampler, input.uv);
}