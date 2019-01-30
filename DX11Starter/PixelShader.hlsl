
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
	float3 tangent		: TANGENT0;
	float3 bitangent	: TANGENT1;
};

struct DirectionalLight
{
	float4 AmbientColor;
	float4 DiffuseColor;
	float4 SpecularColor;
	float3 Direction;
};

cbuffer externalData : register(b0)
{
	DirectionalLight light0;
};

cbuffer materialData : register(b1)
{
	// Material Data
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float4 emission;
	float shininess;

	float turnOnNormal;
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
	float3 l = normalize(light0.Direction);
	float3 h = normalize(l + v);

	float3 t = normalize(input.tangent - dot(input.tangent, n) * n);
	float3 b = normalize(cross(n, t));

	float4 result = float4(0, 0, 0, 0);

	// Normal Mapping
	float4 normalMapping = normalTexture.Sample(basicSampler, input.uv);
	normalMapping = normalMapping * 2 - 1;

	float3x3 TBN = float3x3(t, b, n);

	if (turnOnNormal)
		n = mul(normalMapping.xyz, TBN);
	n = normalize(n);

	// Adjust the variables below as necessary to work with your own code
	float4 surfaceColor = diffuseTexture.Sample(basicSampler, input.uv);

	result += light0.AmbientColor * surfaceColor;

	float ndl = dot(n, l);
	ndl = max(ndl, 0);

	float4 diffuseColor = (ndl * light0.DiffuseColor * surfaceColor);
	diffuseColor.w = 0;
	result += diffuseColor * diffuse;

	// Blinn-Phong
	float ndh = dot(n, h);
	ndh = max(ndh, 0);
	float3 specularColor3 = saturate(pow(ndh, max(shininess, 10.0)) * light0.SpecularColor.xyz);
	float4 specularColor;
	specularColor.xyz = specularColor3;
	specularColor.w = 0;
	result += specularColor * specular;

	result = saturate(result);
	return result;
	//return float4(n, 1.0);
	//return diffuse;
}