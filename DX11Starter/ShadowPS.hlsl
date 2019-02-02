// A dummy shader

struct VertexToPixel
{
	float4 pos		: SV_POSITION;
};


float4 main(VertexToPixel input) : SV_TARGET
{
	discard;
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}