#define MAX_LIGHTS 24

// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	float4 position				: SV_POSITION;	// XYZW position (System Value Position)
	float4 worldPos				: POSITION0;
	float3 normal				: NORMAL;
	float2 uv					: TEXCOORD;
	float3 tangent				: TANGENT;
	float4 lViewSpacePos		: POSITION1;
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
	float3 AmbientColor;// 64 bytes
};

struct Material
{
	// Material Data
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float4 emission;
	float shininess;
};

cbuffer externalData : register(b0)
{
	Light lights[MAX_LIGHTS];
	int lightCount;
};

cbuffer materialData : register(b1)
{
	Material material;

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
	float3 l;
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
	{
		surfaceColor = diffuseTexture.Sample(basicSampler, input.uv);
		// Gamma correction
		//surfaceColor.xyz = pow(surfaceColor.xyz, 2.2);
	}

	else
		surfaceColor = float4(1.0, 1.0, 1.0, 1.0);

	float spotAmount = 1.0;
	float4 ambientColor;
	ambientColor.w = surfaceColor.w;

	for (int i = 0; i < lightCount; ++i)
	{
		float intensity = lights[i].Intensity;
		[call] switch (lights[i].Type)
		{
		case 0:
			// Directional
			l = normalize(lights[i].Direction);
			ambientColor.xyz = lights[i].AmbientColor;
			break;
		case 1:
			// Point
			{
				l = lights[i].Position - input.worldPos.xyz;
				float dist = length(l);
				float att = saturate(1.0 - dist * dist / (lights[i].Range * lights[i].Range));
				att *= att;
				intensity = att * intensity;
				l = normalize(l);
				ambientColor.xyz = float3(0, 0, 0);
			}
			break;
		case 2:
			// Spot
			{
				l = lights[i].Position - input.worldPos.xyz;
				float dist = length(l);
				float att = saturate(1.0 - dist * dist / (lights[i].Range * lights[i].Range));
				att *= att;
				intensity = att * intensity;
				l = normalize(l);
				float angleFromCenter = max(dot(-l, normalize(lights[i].Direction)), 0.0f);
				if (angleFromCenter < lights[i].SpotFalloff)
					intensity = 0.0;
				spotAmount = 1.0 - (1.0 - angleFromCenter) * 1.0 / (1.0 - lights[i].SpotFalloff);
				//intensity = spotAmount * intensity;
				ambientColor.xyz = float3(0, 0, 0);
			}
			break;
		}
		float3 h = normalize(l + v);

		float4 lightColor = float4(lights[i].Color.xyz, 1.0);

		result += intensity * ambientColor * surfaceColor;

		float ndl = dot(n, l);
		ndl = max(ndl, 0);

		float4 diffuseColor = ndl * lightColor * intensity * spotAmount * surfaceColor;
		diffuseColor.w = 0;
		result += diffuseColor * material.diffuse;

		// Blinn-Phong
		float ndh = dot(n, h);
		ndh = max(ndh, 0);
		float3 specularColor3 = saturate(pow(ndh, max(material.shininess, 10.0)) * lights[i].Color * intensity * spotAmount);
		float4 specularColor;
		specularColor.xyz = specularColor3;
		specularColor.w = 0;
		result += specularColor * material.specular;
	}
	// Gamma correction
	//result.xyz = pow(result.xyz, 1.0f / 2.2f);

	result.w = surfaceColor.w;
	result = saturate(result);
	return result;
	//return float4(t, 1.0);
	//return diffuse;
}