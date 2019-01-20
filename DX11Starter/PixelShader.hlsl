
// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float4 color		: COLOR;
	float3 normal		: NORMAL;
	float2 uv			: TEXCOORD;
};

struct DirectionalLight
{
	float4 AmbientColor;
	float4 DiffuseColor;
	float3 Direction;
};

cbuffer externalData : register(b0)
{
	DirectionalLight light0;
};

Texture2D diffuseTexture  : register(t0);
SamplerState basicSampler : register(s0);

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
	input.normal = normalize(input.normal);
	float4 result = float4(0, 0, 0, 0);

	// Adjust the variables below as necessary to work with your own code
	float4 surfaceColor = diffuseTexture.Sample(basicSampler, input.uv);

	float3 normalizedLightDirection = normalize(light0.Direction);
	float diffuseLightIntensity = dot(input.normal, light0.Direction);
	diffuseLightIntensity = saturate(diffuseLightIntensity);

	float4 diffuse = saturate(diffuseLightIntensity * light0.DiffuseColor * surfaceColor);
	result += diffuse + light0.AmbientColor * surfaceColor;

	result = saturate(result);

	return float4(result);
}