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

cbuffer shadowData : register(b3)
{
	//float4x4        cascadeProjection;
	float4          m_vCascadeOffset[3];
	float4          m_vCascadeScale[3];
	int             m_nCascadeLevels; // Number of Cascades
	int             m_iVisualizeCascades; // 1 is to visualize the cascades in different colors. 0 is to just draw the scene
	int             m_iPCFBlurForLoopStart; // For loop begin value. For a 5x5 Kernal this would be -2.
	int             m_iPCFBlurForLoopEnd; // For loop end value. For a 5x5 kernel this would be 3.

	// For Map based selection scheme, this keeps the pixels inside of the the valid range.
	// When there is no border, these values are 0 and 1 respectivley.
	float           m_fMinBorderPadding;
	float           m_fMaxBorderPadding;
	float           m_fShadowBiasFromGUI;  // A shadow map offset to deal with self shadow artifacts.  
										   //These artifacts are aggravated by PCF.
	float           m_fShadowPartitionSize;
	float           m_fCascadeBlendArea; // Amount to overlap when blending between cascades.
	float           m_fTexelSize;
	float           m_fNativeTexelSizeInX;
	float           m_fPaddingForCB3; // Padding variables exist because CBs must be a multiple of 16 bytes.
}

Texture2D diffuseTexture  : register(t0);
Texture2D normalTexture  : register(t1);
TextureCube cubemap : register(t2);
TextureCube irradianceMap : register(t3);
Texture2D shadowMap : register(t4);
SamplerState basicSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

static const float4 vCascadeColorsMultiplier[8] =
{
	float4 (1.5f, 0.0f, 0.0f, 1.0f),
	float4 (0.0f, 1.5f, 0.0f, 1.0f),
	float4 (0.0f, 0.0f, 5.5f, 1.0f),
	float4 (1.5f, 0.0f, 5.5f, 1.0f),
	float4 (1.5f, 1.5f, 0.0f, 1.0f),
	float4 (1.0f, 1.0f, 1.0f, 1.0f),
	float4 (0.0f, 1.0f, 5.5f, 1.0f),
	float4 (0.5f, 3.5f, 0.75f, 1.0f)
};

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

	float G = min(1, min(2 * NdH * NdV / VdH, 2 * NdH * NdL / VdH));

	return G * D * F;
}

//--------------------------------------------------------------------------------------
// Calculate amount to blend between two cascades and the band where blending will occure.
//--------------------------------------------------------------------------------------
void CalculateBlendAmountForMap(in float4 vShadowMapTextureCoord,
	in out float fCurrentPixelsBlendBandLocation,
	out float fBlendBetweenCascadesAmount)
{
	// Calcaulte the blend band for the map based selection.
	float2 distanceToOne = float2 (1.0f - vShadowMapTextureCoord.x, 1.0f - vShadowMapTextureCoord.y);
	fCurrentPixelsBlendBandLocation = min(vShadowMapTextureCoord.x, vShadowMapTextureCoord.y);
	float fCurrentPixelsBlendBandLocation2 = min(distanceToOne.x, distanceToOne.y);
	fCurrentPixelsBlendBandLocation =
		min(fCurrentPixelsBlendBandLocation, fCurrentPixelsBlendBandLocation2);
	fBlendBetweenCascadesAmount = fCurrentPixelsBlendBandLocation / m_fCascadeBlendArea;
}

void ComputeCoordinatesTransform(in int iCascadeIndex,
	in float4 InterpolatedPosition,
	in out float4 vShadowTexCoord,
	in out float4 vShadowTexCoordViewSpace)
{
	// Now that we know the correct map, we can transform the world space position of the current fragment                
	vShadowTexCoord = vShadowTexCoordViewSpace * m_vCascadeScale[iCascadeIndex];
	vShadowTexCoord += m_vCascadeOffset[iCascadeIndex];

	vShadowTexCoord.x *= m_fShadowPartitionSize;  // precomputed (float)iCascadeIndex / (float)CASCADE_CNT
	vShadowTexCoord.x += (m_fShadowPartitionSize * (float)iCascadeIndex);


}

//--------------------------------------------------------------------------------------
// This function calculates the screen space depth for shadow space texels
//--------------------------------------------------------------------------------------
void CalculateRightAndUpTexelDepthDeltas(in float3 vShadowTexDDX,
	in float3 vShadowTexDDY,
	out float fUpTextDepthWeight,
	out float fRightTextDepthWeight
) {

	// We use the derivatives in X and Y to create a transformation matrix.  Because these derivives give us the 
	// transformation from screen space to shadow space, we need the inverse matrix to take us from shadow space 
	// to screen space.  This new matrix will allow us to map shadow map texels to screen space.  This will allow 
	// us to find the screen space depth of a corresponding depth pixel.
	// This is not a perfect solution as it assumes the underlying geometry of the scene is a plane.  A more 
	// accureate way of finding the actual depth would be to do a deferred rendering approach and actually 
	//sample the depth.

	// Using an offset, or using variance shadow maps is a better approach to reducing these artifacts in most cases.

	float2x2 matScreentoShadow = float2x2(vShadowTexDDX.xy, vShadowTexDDY.xy);
	float fDeterminant = determinant(matScreentoShadow);

	float fInvDeterminant = 1.0f / fDeterminant;

	float2x2 matShadowToScreen = float2x2 (
		matScreentoShadow._22 * fInvDeterminant, matScreentoShadow._12 * -fInvDeterminant,
		matScreentoShadow._21 * -fInvDeterminant, matScreentoShadow._11 * fInvDeterminant);

	float2 vRightShadowTexelLocation = float2(m_fTexelSize, 0.0f);
	float2 vUpShadowTexelLocation = float2(0.0f, m_fTexelSize);

	// Transform the right pixel by the shadow space to screen space matrix.
	float2 vRightTexelDepthRatio = mul(vRightShadowTexelLocation, matShadowToScreen);
	float2 vUpTexelDepthRatio = mul(vUpShadowTexelLocation, matShadowToScreen);

	// We can now caculate how much depth changes when you move up or right in the shadow map.
	// We use the ratio of change in x and y times the dervivite in X and Y of the screen space 
	// depth to calculate this change.
	fUpTextDepthWeight =
		vUpTexelDepthRatio.x * vShadowTexDDX.z
		+ vUpTexelDepthRatio.y * vShadowTexDDY.z;
	fRightTextDepthWeight =
		vRightTexelDepthRatio.x * vShadowTexDDX.z
		+ vRightTexelDepthRatio.y * vShadowTexDDY.z;

}

//--------------------------------------------------------------------------------------
// Use PCF to sample the depth map and return a percent lit value.
//--------------------------------------------------------------------------------------
void CalculatePCFPercentLit(in float4 vShadowTexCoord,
	in float fRightTexelDepthDelta,
	in float fUpTexelDepthDelta,
	in float fBlurRowSize,
	out float fPercentLit
)
{
	fPercentLit = 0.0f;
	// This loop could be unrolled, and texture immediate offsets could be used if the kernel size were fixed.
	// This would be performance improvment.
	for (int x = m_iPCFBlurForLoopStart; x < m_iPCFBlurForLoopEnd; ++x)
	{
		for (int y = m_iPCFBlurForLoopStart; y < m_iPCFBlurForLoopEnd; ++y)
		{
			float depthcompare = vShadowTexCoord.z;
			// A very simple solution to the depth bias problems of PCF is to use an offset.
			// Unfortunately, too much offset can lead to Peter-panning (shadows near the base of object disappear )
			// Too little offset can lead to shadow acne ( objects that should not be in shadow are partially self shadowed ).
			depthcompare -= m_fShadowBiasFromGUI;

			// Add in derivative computed depth scale based on the x and y pixel.
			depthcompare += fRightTexelDepthDelta * ((float)x) + fUpTexelDepthDelta * ((float)y);

			// Compare the transformed pixel depth to the depth read from the map.
			fPercentLit += shadowMap.SampleCmpLevelZero(shadowSampler,
				float2(
					vShadowTexCoord.x + (((float)x) * m_fNativeTexelSizeInX),
					vShadowTexCoord.y + (((float)y) * m_fTexelSize)
					),
				depthcompare);
		}
	}
	fPercentLit /= (float)fBlurRowSize;
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
			l = normalize(-lights[i].Direction);
			break;
		case 1:
			// Point
			{
				l = input.worldPos.xyz - lights[i].Position;
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
				l = input.worldPos.xyz - lights[i].Position;
				float dist = length(l);
				float att = saturate(1.0 - dist * dist / (lights[i].Range * lights[i].Range));
				att *= att;
				intensity = att * intensity;
				l = normalize(l);
				float angleFromCenter = max(dot(l, normalize(lights[i].Direction)), 0.0f);
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
		float4 vVisualizeCascadeColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
		if (i == 0)
		{
			// Only cast shadow for light 0
			//float2 shadowTexCoords;
			//float4 lSpacePos = mul(input.lViewSpacePos, cascadeProjection);
			//shadowTexCoords.x = 0.5f + (lSpacePos.x / lSpacePos.w * 0.5f);
			//shadowTexCoords.y = 0.5f - (lSpacePos.y / lSpacePos.w * 0.5f);
			//float pixelDepth = lSpacePos.z / lSpacePos.w;

			//// Check if the pixel texture coordinate is in the view frustum of the 
			//// light before doing any shadow work.
			//if ((saturate(shadowTexCoords.x) == shadowTexCoords.x) &&
			//	(saturate(shadowTexCoords.y) == shadowTexCoords.y) &&
			//	(pixelDepth > 0))
			//{
			//	// Use the SampleCmpLevelZero Texture2D method (or SampleCmp) to sample from 
			//	// the shadow map.
			//	lighting = float(shadowMap.SampleCmpLevelZero(
			//		shadowSampler,
			//		shadowTexCoords,
			//		pixelDepth));
			//}






			float4 vShadowMapTextureCoord = 0.0f;
			float4 vShadowMapTextureCoord_blend = 0.0f;



			float fPercentLit = 0.0f;
			float fPercentLit_blend = 0.0f;


			float fUpTextDepthWeight = 0;
			float fRightTextDepthWeight = 0;
			float fUpTextDepthWeight_blend = 0;
			float fRightTextDepthWeight_blend = 0;

			int iBlurRowSize = m_iPCFBlurForLoopEnd - m_iPCFBlurForLoopStart;
			iBlurRowSize *= iBlurRowSize;
			float fBlurRowSize = (float)iBlurRowSize;

			int iCascadeFound = 0;
			int iNextCascadeIndex = 1;

			float fCurrentPixelDepth;

			int iCurrentCascadeIndex;

			float4 vShadowMapTextureCoordViewSpace = input.lViewSpacePos;

			iCurrentCascadeIndex = 0;

			for (int iCascadeIndex = 0; iCascadeIndex < m_nCascadeLevels && iCascadeFound == 0; ++iCascadeIndex)
			{
				vShadowMapTextureCoord = vShadowMapTextureCoordViewSpace * m_vCascadeScale[iCascadeIndex];
				vShadowMapTextureCoord += m_vCascadeOffset[iCascadeIndex];

				if (min(vShadowMapTextureCoord.x, vShadowMapTextureCoord.y) > m_fMinBorderPadding
					&& max(vShadowMapTextureCoord.x, vShadowMapTextureCoord.y) < m_fMaxBorderPadding)
				{
					iCurrentCascadeIndex = iCascadeIndex;
					iCascadeFound = 1;
				}
			}

			float4 color = 0;

			// Repeat text coord calculations for the next cascade. 
			// The next cascade index is used for blurring between maps.
			iNextCascadeIndex = min(m_nCascadeLevels - 1, iCurrentCascadeIndex + 1);

			float fBlendBetweenCascadesAmount = 1.0f;
			float fCurrentPixelsBlendBandLocation = 1.0f;

			CalculateBlendAmountForMap(vShadowMapTextureCoord,
				fCurrentPixelsBlendBandLocation, fBlendBetweenCascadesAmount);

			float3 vShadowMapTextureCoordDDX;
			float3 vShadowMapTextureCoordDDY;
			// The derivatives are used to find the slope of the current plane.
			// The derivative calculation has to be inside of the loop in order to prevent divergent flow control artifacts.
			vShadowMapTextureCoordDDX = ddx(vShadowMapTextureCoordViewSpace);
			vShadowMapTextureCoordDDY = ddy(vShadowMapTextureCoordViewSpace);

			vShadowMapTextureCoordDDX *= m_vCascadeScale[iCurrentCascadeIndex];
			vShadowMapTextureCoordDDY *= m_vCascadeScale[iCurrentCascadeIndex];


			ComputeCoordinatesTransform(iCurrentCascadeIndex,
				input.worldPos,
				vShadowMapTextureCoord,
				vShadowMapTextureCoordViewSpace);


			vVisualizeCascadeColor = vCascadeColorsMultiplier[iCurrentCascadeIndex];

			CalculateRightAndUpTexelDepthDeltas(vShadowMapTextureCoordDDX, vShadowMapTextureCoordDDY,
				fUpTextDepthWeight, fRightTextDepthWeight);

			CalculatePCFPercentLit(vShadowMapTextureCoord, fRightTextDepthWeight,
				fUpTextDepthWeight, fBlurRowSize, fPercentLit);

			if (fCurrentPixelsBlendBandLocation < m_fCascadeBlendArea)
			{  // the current pixel is within the blend band.

				// Repeat text coord calculations for the next cascade. 
				// The next cascade index is used for blurring between maps.
				vShadowMapTextureCoord_blend = vShadowMapTextureCoordViewSpace * m_vCascadeScale[iNextCascadeIndex];
				vShadowMapTextureCoord_blend += m_vCascadeOffset[iNextCascadeIndex];

				ComputeCoordinatesTransform(iNextCascadeIndex, input.worldPos,
					vShadowMapTextureCoord_blend,
					vShadowMapTextureCoordViewSpace);

				// We repeat the calcuation for the next cascade layer, when blending between maps.
				if (fCurrentPixelsBlendBandLocation < m_fCascadeBlendArea)
				{  
					// the current pixel is within the blend band.
					CalculateRightAndUpTexelDepthDeltas(vShadowMapTextureCoordDDX,
						vShadowMapTextureCoordDDY,
						fUpTextDepthWeight_blend,
						fRightTextDepthWeight_blend);

					CalculatePCFPercentLit(vShadowMapTextureCoord_blend, fRightTextDepthWeight_blend,
						fUpTextDepthWeight_blend, fBlurRowSize, fPercentLit_blend);
					fPercentLit = lerp(fPercentLit_blend, fPercentLit, fBlendBetweenCascadesAmount);
					// Blend the two calculated shadows by the blend amount.
				}
			}
			lighting = fPercentLit;
			if (!m_iVisualizeCascades) vVisualizeCascadeColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		float4 lightColor = float4(lights[i].Color.xyz, 0.0);
		intensity = saturate(intensity * spotAmount) * lighting;

		float diffuseFactor = NdL;
		float4 specularColor = NdL * float4(GGX(n, l, v), 0.0f) * lightColor * intensity;

		specular += specularColor;

		float3 diffuseColor = DiffuseEnergyConserve(diffuseFactor, specularColor.xyz, material.metalness) * vVisualizeCascadeColor;
		diffuse += float4(diffuseColor, 0.0) * lightColor * intensity;

	}

	result = surfaceColor * diffuse + specular + float4(IBL(n, v, l), 0.0f);
	result.w = surfaceColor.w;
	result = saturate(result);
	return result;
	//return float4(lighting, lighting, lighting, 1.0);
}