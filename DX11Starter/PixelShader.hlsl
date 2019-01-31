#define MAX_LIGHTS 128

// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float3 normal		: NORMAL;
	float2 uv			: TEXCOORD;
	float3 tangent		: TANGENT;
};

struct Light
{
	int Type;
	float3 Direction;// 16 bytes
	float Range;
	float3 Position;// 32 bytes
	float Intensity;
	float3 Color;// 48 bytes
	float SpotFalloff;
	float3 Padding;// 64 bytes
};

cbuffer externalData : register(b0)
{
	Light lights[MAX_LIGHTS];
	int lightCount;
};

cbuffer materialData : register(b1)
{
	// Material Data
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float4 emission;
	float shininess;

	float hasDiffuseTexture;
	float hasNormalMap;
};

cbuffer cameraData : register(b2)
{
	float3 CameraDirection;
};

Texture2D diffuseTexture  : register(t0);
Texture2D normalTexture  : register(t1);
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
	float3 v = normalize(-CameraDirection);
	float3 n = normalize(input.normal);
	float3 t = normalize(input.tangent - dot(input.tangent, n) * n);
	float3 b = normalize(cross(n, t));

	float4 result = float4(0, 0, 0, 0);

	if (hasNormalMap)
	{
		// Normal Mapping
		float4 normalMapping = normalTexture.Sample(basicSampler, input.uv);
		normalMapping = normalMapping * 2 - 1;

		float3x3 TBN = float3x3(t, b, n);

		n = mul(normalMapping.xyz, TBN);
	}
	n = normalize(n);

	float4 surfaceColor;

	if (hasDiffuseTexture)
		surfaceColor = diffuseTexture.Sample(basicSampler, input.uv);
	else
		surfaceColor = float4(1.0, 1.0, 1.0, 1.0);


	for (int i = 0; i < lightCount; ++i)
	{
		float3 l = normalize(lights[i].Direction);
		float3 h = normalize(l + v);

		float4 lightColor = float4(lights[i].Color.xyz, 1.0);

		result += lights[i].Intensity * 0.8 * surfaceColor;

		float ndl = dot(n, l);
		ndl = max(ndl, 0);

		float4 diffuseColor = ndl * lightColor * lights[i].Intensity * surfaceColor;
		diffuseColor.w = 0;
		result += diffuseColor * diffuse;

		// Blinn-Phong
		float ndh = dot(n, h);
		ndh = max(ndh, 0);
		float3 specularColor3 = saturate(pow(ndh, max(shininess, 10.0)) * lights[i].Color * lights[i].Intensity);
		float4 specularColor;
		specularColor.xyz = specularColor3;
		specularColor.w = 0;
		result += specularColor * specular;
	}
	result.w = surfaceColor.w;
	result = saturate(result);
	return result;
	//return float4(t, 1.0);
	//return diffuse;
}