#define MAX_LIGHTS 24
#define FLT_EPSILON 1.19209290E-07F

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
	float4 lSpacePos			: POSITION1;
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
TextureCube cubemap : register(t2);
TextureCube irradianceMap : register(t3);
Texture2D shadowMap : register(t4);
SamplerState basicSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

// https://www.knaldtech.com/docs/doku.php?id=specular_lys
static const int nMipOffset = 0;

float RoughnessFromPerceptualRoughness(float fPerceptualRoughness)
{
	return fPerceptualRoughness * fPerceptualRoughness;
}

float PerceptualRoughnessFromRoughness(float fRoughness)
{
	return sqrt(max(0.0, fRoughness));
}

float SpecularPowerFromPerceptualRoughness(float fPerceptualRoughness)
{
	float fRoughness = RoughnessFromPerceptualRoughness(fPerceptualRoughness);
	return (2.0 / max(FLT_EPSILON, fRoughness * fRoughness)) - 2.0;
}

float PerceptualRoughnessFromSpecularPower(float fSpecPower)
{
	float fRoughness = sqrt(2.0 / (fSpecPower + 2.0));
	return PerceptualRoughnessFromRoughness(fRoughness);
}

float BurleyToMip(float fPerceptualRoughness, int nMips, float NdR)
{
	float fSpecPower = SpecularPowerFromPerceptualRoughness(fPerceptualRoughness);
	fSpecPower /= (4 * max(NdR, FLT_EPSILON));
	float fScale = PerceptualRoughnessFromSpecularPower(fSpecPower);
	return fScale * (nMips - 1 - nMipOffset);
}

float3 DiffuseEnergyConserve(float diffuse, float3 specular, float metalness)
{
	return diffuse * ((1 - saturate(specular)) * (1 - metalness));
}

float3 IBL(float3 n, float3 v, float3 l)
{
	float3 r = normalize(reflect(-v, n));
	float NdV = max(dot(n, v), 0.0);
	float NdL = max(dot(n, l), 0.0);
	float NdR = max(dot(n, r), 0.0);

	int mipLevels, width, height;
	cubemap.GetDimensions(0, width, height, mipLevels);

	float3 diffuseImageLighting = irradianceMap.Sample(basicSampler, mul(float4(n, 0.0), SkyboxRotation).xyz).rgb;
	float3 specularImageLighting = cubemap.SampleLevel(basicSampler, mul(float4(r, 0.0), SkyboxRotation).xyz, BurleyToMip(pow(material.roughness, 0.5), mipLevels, NdR)).rgb;

	float4 specularColor = float4(lerp(0.04f.rrr, material.albedo, material.metalness), 1.0f);
	float4 schlickFresnel = saturate(specularColor + (1 - specularColor) * pow(1 - NdV, 5));

	float3 diffuseResult = diffuseImageLighting * material.albedo;
	float3 result = lerp(diffuseResult, specularImageLighting * material.albedo, schlickFresnel.xyz);

	return result;
}

float FresnelSchlick(float f0, float fd90, float view)
{
	return f0 + (fd90 - f0) * pow(max(1.0f - view, 0.1f), 5.0f);
}

float3 GGX(float3 n, float3 l, float3 v)
{
	float3 h = normalize(l + v);
	float NdH = saturate(dot(n, h));

	float rough2 = material.roughness * material.roughness;
	float rough4 = max(rough2 * rough2, FLT_EPSILON);

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
	float3 originalN = n;
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

		float lighting = 1;

		if (i == 0)
		{
			float2 shadowTexCoords;
			shadowTexCoords.x = 0.5f + (input.lSpacePos.x / input.lSpacePos.w * 0.5f);
			shadowTexCoords.y = 0.5f - (input.lSpacePos.y / input.lSpacePos.w * 0.5f);
			float pixelDepth = input.lSpacePos.z / input.lSpacePos.w;

			// Check if the pixel texture coordinate is in the view frustum of the 
			// light before doing any shadow work.
			if ((saturate(shadowTexCoords.x) == shadowTexCoords.x) &&
				(saturate(shadowTexCoords.y) == shadowTexCoords.y) &&
				(pixelDepth > 0))
			{
				// Use the SampleCmpLevelZero Texture2D method (or SampleCmp) to sample from 
				// the shadow map, just as you would with Direct3D feature level 10_0 and
				// higher.  Feature level 9_1 only supports LessOrEqual, which returns 0 if
				// the pixel is in the shadow.
				lighting = float(shadowMap.SampleCmpLevelZero(
					shadowSampler,
					shadowTexCoords,
					pixelDepth));
			}
		}

		float4 lightColor = float4(lights[i].Color.xyz, 0.0);
		intensity = saturate(intensity * spotAmount) * lighting;

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
	//return float4(lighting, lighting, lighting, 1.0);
}