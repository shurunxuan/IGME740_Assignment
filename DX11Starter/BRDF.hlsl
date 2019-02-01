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
	float3 albedo;
	float roughness;
	float metalness;
};

cbuffer lightData : register(b0)
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
	float4x4 SkyboxRotation;
};

Texture2D diffuseTexture  : register(t0);
Texture2D normalTexture  : register(t1);
Texture2D preIntegrated : register(t2);
TextureCube cubemap : register(t3);
TextureCube irradianceMap : register(t4);
SamplerState basicSampler : register(s0);
SamplerState anisotropic : register(s1);

// Lys constants
static const float k0 = 0.00098, k1 = 0.9921, fUserMaxSPow = 0.2425;
static const float g_fMaxT = (exp2(-10.0 / sqrt(fUserMaxSPow)) - k0) / k1;
static const int nMipOffset = 0;

// Lys function, copy/pasted from https://www.knaldtech.com/docs/doku.php?id=specular_lys
float GetSpecPowToMip(float fSpecPow, int nMips)
{
	// This line was added because ShaderTool destroys mipmaps.
	fSpecPow = 1 - pow(1 - fSpecPow, 4);
	// Default curve - Inverse of Toolbag 2 curve with adjusted constants.
	float fSmulMaxT = (exp2(-10.0 / sqrt(fSpecPow)) - k0) / k1;
	return float(nMips - 1 - nMipOffset) * (1.0 - clamp(fSmulMaxT / g_fMaxT, 0.0, 1.0));
}

float3 DiffuseEnergyConserve(float diffuse, float3 specular, float metalness)
{
	return diffuse * ((1 - saturate(specular)) * (1 - metalness));
}

float3 RadianceIBLIntegration(float NdV, float roughness)
{
	float2 preintegratedFG = preIntegrated.Sample(anisotropic, float2(roughness, 1.0f - NdV)).rg;
	return material.albedo * preintegratedFG.r + preintegratedFG.g;
}

float3 IBL(float3 n, float3 v, float3 l)
{
	float NdV = max(dot(n, v), 0.0);
	float NdL = max(dot(n, l), 0.0);

	float3 refVec = normalize(reflect(-v, n));

	int mipLevels, width, height;
	cubemap.GetDimensions(0, width, height, mipLevels);

	float3 result = RadianceIBLIntegration(NdV, material.roughness);

	float3 diffuseImageLighting = irradianceMap.Sample(basicSampler, mul(float4(n, 0.0), SkyboxRotation).xyz).rgb;
	float3 specularImageLighting = cubemap.SampleLevel(basicSampler, mul(float4(refVec, 0.0), SkyboxRotation).xyz, GetSpecPowToMip(material.roughness, mipLevels)).rgb;

	float4 specularColor = float4(lerp(0.04f.rrr, material.albedo, material.metalness), 1.0f);
	float4 schlickFresnel = saturate(specularColor + (1 - specularColor) * pow(1 - NdV, 5));

	float3 diffuseResult = diffuseImageLighting * material.albedo;
	float3 specularResult = lerp(diffuseResult, specularImageLighting * material.albedo, schlickFresnel);

	return result + specularResult;
}

float FresnelSchlick(float f0, float fd90, float view)
{
	return f0 + (fd90 - f0) * pow(max(1.0f - view, 0.1f), 5.0f);
}

float3 GGX(float3 n, float3 l, float3 v)
{
	float3 h = normalize(l + v);
	float NdH = saturate(dot(n, h));

	float rough2 = max(material.roughness * material.roughness, 0.002);
	float rough4 = rough2 * rough2;

	float d = (NdH * rough4 - NdH) * NdH + 1.0f;
	float D = rough4 / (3.14159265 * (d * d));

	float3 specularColor = lerp(0.04, material.albedo, material.metalness);
	float3 reflectivity = specularColor;
	float fresnel = 1.0;

	float NdL = saturate(dot(n, l));
	float LdH = saturate(dot(l, h));
	float NdV = saturate(dot(n, v));
	float VdH = saturate(dot(v, h));

	float3 F = reflectivity + (float3(1.0, 1.0, 1.0) - reflectivity) * pow(1 - LdH, 5);

	//float k = rough2 * 0.5f;
	//float G_SmithL = NdL / (NdL * (1.0f - k) + k);
	//float G_SmithV = NdV / (NdV * (1.0f - k) + k);
	//float G = (G_SmithL * G_SmithV);

	float G = min(1, min(2 * NdH * NdV / VdH, 2 * NdH * NdL / VdH));

	return G * D * F;
}

float4 main(VertexToPixel input) : SV_TARGET
{
	float3 v = normalize(-CameraDirection);
	float3 n = normalize(input.normal);
	float3 t = normalize(input.tangent - dot(input.tangent, n) * n);
	float3 b = normalize(cross(n, t));
	float3 l;
	float4 result = float4(0, 0, 0, 0);
	float4 directLighting = float4(0, 0, 0, 0);
	float4 indirectLighting = float4(0, 0, 0, 0);

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
		surfaceColor = diffuseTexture.Sample(basicSampler, input.uv) * float4(material.albedo.xyz, 1.0);
		// Gamma correction
		//surfaceColor.xyz = pow(surfaceColor.xyz, 2.2);
	}
	else
		surfaceColor = float4(material.albedo.xyz, 1.0);

	float spotAmount = 1.0;

	// Cubemap Reflection
	float4 sampleDirection = float4(reflect(-v, n), 0.0);

	float4 diffuse = float4(0, 0, 0, 0);
	float4 specular = float4(0, 0, 0, 0);

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
				spotAmount = saturate(spotAmount);
				//intensity = spotAmount * intensity;
			}
			break;
		}
		float3 h = normalize(l + v);
		float NdL = saturate(dot(n, l));
		float NdV = saturate(dot(n, v));

		float4 lightColor = float4(lights[i].Color.xyz, 0.0);
		intensity = saturate(intensity * spotAmount);

		float diffuseFactor = NdL;
		float4 specularColor = NdL * float4(GGX(n, l, v), 0.0f) * lightColor * intensity;

		specular += specularColor;

		float3 diffuseColor = DiffuseEnergyConserve(diffuseFactor, specularColor.xyz, material.metalness);
		diffuse += float4(diffuseColor, 0.0) * lightColor * intensity;
	}

	result = surfaceColor * diffuse + specular + float4(IBL(n, v, l), 0.0f);
	result.w = surfaceColor.w;
	result = saturate(result);
	return result;
	//return diffuse;
}