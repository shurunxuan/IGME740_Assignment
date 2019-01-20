struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float4 color		: COLOR;
	float3 normal		: NORMAL;
};

float4 main(VertexToPixel input) : SV_TARGET
{
	return float4(input.color);
}