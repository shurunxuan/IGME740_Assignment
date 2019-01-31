#define MAX_LIGHTS 128

// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float4 worldPos		: POSITION;
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
	float3 AmbientColor;// 64 bytes
};

struct Material
{
	// Material Data
	float3 reflectance;
	float roughness;
	float metalness;
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

float SpecDistribution(float3 n, float3 h, float roughness)
{
	// Pre-calculations
	float NdotH = saturate(dot(n, h));
	float NdotH2 = NdotH * NdotH;
	float a = roughness * roughness; // Remap roughness (Unreal)
	float a2 = max(a * a, 0.00001); // Applied after remap!

	// ((n dot h)^2 * (a^2 - 1) + 1)
	float denomToSquare = NdotH2 * (a2 - 1) + 1;
	// Can go to zero if roughness is 0 and NdotH is 1

	// Final value
	return a2 / (3.14159265 * denomToSquare * denomToSquare);
}

// f0 ranges from 0.04 for non-metals to
// a specific specular color for metals
float3 Fresnel(float3 v, float3 h, float3 f0)
{
	// Pre-calculations
	float VdotH = saturate(dot(v, h));

	// Final value
	return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

// k is remapped to a / 2 (a is roughness^2)
// roughness remapped to (r+1)/2
float GeometricShadowing(
	float NdV, float3 h, float roughness)
{
	// End result of remapping:
	float k = pow(roughness + 1, 2) / 8.0f;
	float NdotV = saturate(NdV);

	// Final value
	return NdotV / (NdotV * (1 - k) + k);
}

float3 MicroFacet(float NdL, float3 n, float3 v, float3 l, float3 h, float roughness, float3 reflectance)
{
	float NdV = dot(n, v);
	// Grab various functions
	float  D = SpecDistribution(n, h, roughness);
	float3 F = Fresnel(v, h, reflectance);
	float  G =
		GeometricShadowing(NdV, h, roughness) *
		GeometricShadowing(NdL, h, roughness);

	// Final formula
	// Note: NdotV or NdotL may go to zero!
	// Some implementations use max(NdotV, NdotL)
	return
		(D * F * G) / (4 * max(NdV, NdL));
}

float3 DiffuseEnergyConserve(
	float diffuse, float3 specular, float metalness)
{
	return
		diffuse *
		((1 - saturate(specular)) * (1 - metalness));
}



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
		surfaceColor = diffuseTexture.Sample(basicSampler, input.uv) * float4(material.reflectance.xyz, 1.0);
		// Gamma correction
		//surfaceColor.xyz = pow(surfaceColor.xyz, 2.2);
	}

	else
		surfaceColor = float4(material.reflectance.xyz, 1.0);

	float spotAmount = 1.0;

	for (int i = 0; i < lightCount; ++i)
	{
		float intensity = lights[i].Intensity;
		[call] switch (lights[i].Type)
		{
		case 0:
			// Directional
			l = normalize(lights[i].Direction);
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
			}
			break;
		}
		float3 h = normalize(l + v);

		float4 lightColor = float4(lights[i].Color.xyz, 1.0);
		lightColor = lightColor * intensity * spotAmount;
		float ndl = dot(n, l);
		ndl = saturate(ndl);

		// BRDF
		float ndh = dot(n, h);
		ndh = max(ndh, 0);
		float3 specularColor = MicroFacet(ndl, n, v, l, h, material.roughness, material.reflectance);

		// Diffuse energy conservation
		float3 energyConserveDiffuse = DiffuseEnergyConserve(ndl, specularColor.xyz, material.metalness) * surfaceColor.xyz;

		result += (float4(energyConserveDiffuse, 0.0) * surfaceColor + float4(specularColor, 0.0)) * lightColor;
	}
	// Gamma correction
	//result.xyz = pow(result.xyz, 1.0f / 2.2f);

	result.w = surfaceColor.w;
	result = saturate(result);
	return result;
	//return float4(t, 1.0);
	//return diffuse;
}