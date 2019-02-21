#include "Light.h"
#include "SimpleLogger.h"
#include <DirectXCollision.h>

Light::Light(LightStructure* data, ID3D11Device* d, ID3D11DeviceContext* c, FirstPersonCamera* cam, DirectX::XMVECTOR aabbMin, DirectX::XMVECTOR aabbMax)
{
	shadowMapDimension = 2048;
	Data = data;

	device = d;
	context = c;
	camera = cam;

	sceneAABBMin = aabbMin;
	sceneAABBMax = aabbMax;

	cascadePartitionsZeroToOne[0] = 3;
	cascadePartitionsZeroToOne[1] = 6;
	cascadePartitionsZeroToOne[2] = 15;

	shadowMap = nullptr;
	shadowDepthView = nullptr;
	shadowResourceView = nullptr;

	D3D11_TEXTURE2D_DESC shadowMapDesc;
	ZeroMemory(&shadowMapDesc, sizeof(D3D11_TEXTURE2D_DESC));
	shadowMapDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	shadowMapDesc.MipLevels = 1;
	shadowMapDesc.ArraySize = 1;
	shadowMapDesc.SampleDesc.Count = 1;
	shadowMapDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	shadowMapDesc.Height = shadowMapDimension;
	shadowMapDesc.Width = shadowMapDimension * 3;

	HRESULT hr = device->CreateTexture2D(
		&shadowMapDesc,
		nullptr,
		&shadowMap
	);

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	hr = device->CreateDepthStencilView(
		shadowMap,
		&depthStencilViewDesc,
		&shadowDepthView
	);

	hr = device->CreateShaderResourceView(
		shadowMap,
		&shaderResourceViewDesc,
		&shadowResourceView
	);

	// Init viewport for shadow rendering
	for (int cascade = 0; cascade < 3; ++cascade)
	{
		shadowViewport[cascade] = new D3D11_VIEWPORT;
		ZeroMemory(shadowViewport[cascade], sizeof(D3D11_VIEWPORT));
		shadowViewport[cascade]->Height = float(shadowMapDimension);
		shadowViewport[cascade]->Width = float(shadowMapDimension);
		shadowViewport[cascade]->MinDepth = 0.f;
		shadowViewport[cascade]->MaxDepth = 1.f;
		shadowViewport[cascade]->TopLeftX = float(shadowMapDimension * cascade);
		shadowViewport[cascade]->TopLeftY = 0;
	}

	UpdateMatrices();

	LOG_INFO << "Light created at <0x" << this << ">." << std::endl;
}

Light::~Light()
{
	if (shadowMap) { shadowMap->Release(); }
	if (shadowDepthView) { shadowDepthView->Release(); }
	if (shadowResourceView) { shadowResourceView->Release(); }

	for (int cascade = 0; cascade < 3; ++cascade)
	{
		delete shadowViewport[cascade];
	}
	LOG_INFO << "Light destroyed at <0x" << this << ">." << std::endl;
}

ID3D11Texture2D* Light::GetShadowMap() const
{
	return shadowMap;
}

ID3D11DepthStencilView* Light::GetShadowDepthView() const
{
	return shadowDepthView;
}

ID3D11ShaderResourceView* Light::GetShadowResourceView() const
{
	return shadowResourceView;
}

D3D11_VIEWPORT* Light::GetShadowViewportAt(int cascade) const
{
	return shadowViewport[cascade];
}

DirectX::XMMATRIX Light::GetViewMatrix() const
{
	return view;
}

int Light::GetCascadeCount() const
{
	return 3;
}

DirectX::XMMATRIX Light::GetProjectionMatrixAt(int index) const
{
	return projection[index];
}

const LightStructure* Light::GetData() const
{
	return Data;
}

void Light::SetDirection(DirectX::XMFLOAT3 d) const
{
	Data->Direction = d;
}

void Light::SetPosition(DirectX::XMFLOAT3 p) const
{
	Data->Position = p;
}

void Light::SetColor(DirectX::XMFLOAT3 c) const
{
	Data->Color = c;
}

void Light::SetAmbientColor(DirectX::XMFLOAT3 a) const
{
	Data->AmbientColor = a;
}

void Light::SetRange(float r) const
{
	Data->Range = r;
}

void Light::SetIntensity(float i) const
{
	Data->Intensity = i;
}

void Light::SetSpotFallOff(float s) const
{
	Data->SpotFalloff = s;
}

void Light::UpdateMatrices()
{
	switch (Data->Type)
	{
	case LIGHT_TYPE_DIR:
		CalculateDirectionalFrustumMatrices();
		break;
	case LIGHT_TYPE_POINT:

		break;
	case LIGHT_TYPE_SPOT:

		break;
	default:
		assert(true);
		break;
	}
}

void Light::CalculateDirectionalFrustumMatrices()
{
	const DirectX::XMVECTOR FLTMAX = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
	const DirectX::XMVECTOR FLTMIN = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
	const DirectX::XMVECTOR halfVector = { 0.5f, 0.5f, 0.5f, 0.5f };

	const DirectX::XMVECTOR eyeDirection = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&Data->Direction));
	const DirectX::XMFLOAT3 cPos = camera->GetPosition();
	const DirectX::XMVECTOR cameraPos = DirectX::XMLoadFloat3(&cPos);
	const DirectX::XMVECTOR eyePosition = DirectX::XMVectorSubtract(cameraPos, DirectX::XMVectorScale(eyeDirection, 100.0f));
	const DirectX::XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
	const DirectX::XMVECTOR upDirection = DirectX::XMLoadFloat3(&up);
	view = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookToLH(eyePosition, eyeDirection, upDirection));


	DirectX::XMMATRIX matViewCameraProjection = DirectX::XMMatrixTranspose(camera->GetProjectionMatrix());
	DirectX::XMMATRIX matViewCameraView = DirectX::XMMatrixTranspose(camera->GetViewMatrix());
	DirectX::XMMATRIX matLightCameraView = DirectX::XMMatrixTranspose(view);

	DirectX::XMMATRIX matInverseViewCamera = DirectX::XMMatrixInverse(nullptr, matViewCameraView);

	// Convert from min max representation to center extents representation.
	// This will make it easier to pull the points out of the transformation.
	DirectX::BoundingBox bb;
	DirectX::BoundingBox::CreateFromPoints(bb, sceneAABBMin, sceneAABBMax);

	DirectX::XMFLOAT3 tmp[8];
	bb.GetCorners(tmp);

	// Transform the scene AABB to Light space.
	DirectX::XMVECTOR sceneAABBPointsLightSpace[8];
	for (int index = 0; index < 8; ++index)
	{
		DirectX::XMVECTOR v = XMLoadFloat3(&tmp[index]);
		sceneAABBPointsLightSpace[index] = XMVector3Transform(v, matLightCameraView);
	}

	float frustumIntervalBegin, frustumIntervalEnd;
	DirectX::XMVECTOR lightCameraOrthographicMin;  // light space frustum aabb 
	DirectX::XMVECTOR lightCameraOrthographicMax;
	float cameraNearFarRange = camera->GetFarClip() - camera->GetNearClip();

	DirectX::XMVECTOR worldUnitsPerTexel = { 0.0f, 0.0f, 0.0f, 0.0f };

	for (int cascadeIndex = 0; cascadeIndex < 3; ++cascadeIndex)
	{
		// Calculate the interval of the View Frustum that this cascade covers. We measure the interval 
		// the cascade covers as a Min and Max distance along the Z Axis.

		// Because we want to fit the orthogrpahic projection tightly around the Cascade, we set the Mimiumum cascade 
		// value to the previous Frustum end Interval
		if (cascadeIndex == 0) frustumIntervalBegin = 0.0f;
		else frustumIntervalBegin = float(cascadePartitionsZeroToOne[cascadeIndex - 1]);


		// Scale the intervals between 0 and 1. They are now percentages that we can scale with.
		frustumIntervalEnd = float(cascadePartitionsZeroToOne[cascadeIndex]);
		frustumIntervalBegin /= float(cascadePartitionsMax);
		frustumIntervalEnd /= float(cascadePartitionsMax);
		frustumIntervalBegin = frustumIntervalBegin * cameraNearFarRange;
		frustumIntervalEnd = frustumIntervalEnd * cameraNearFarRange;
		DirectX::XMVECTOR frustumPoints[8];

		// This function takes the began and end intervals along with the projection matrix and returns the 8
		// points that repreresent the cascade Interval
		CreateFrustumPointsFromCascadeInterval(frustumIntervalBegin, frustumIntervalEnd,
			matViewCameraProjection, frustumPoints);

		lightCameraOrthographicMin = FLTMAX;
		lightCameraOrthographicMax = FLTMIN;

		DirectX::XMVECTOR tempTranslatedCornerPoint;
		// This next section of code calculates the min and max values for the orthographic projection.
		for (int icpIndex = 0; icpIndex < 8; ++icpIndex)
		{
			// Transform the frustum from camera view space to world space.
			frustumPoints[icpIndex] = XMVector4Transform(frustumPoints[icpIndex], matInverseViewCamera);
			// Transform the point from world space to Light Camera Space.
			tempTranslatedCornerPoint = XMVector4Transform(frustumPoints[icpIndex], matLightCameraView);
			// Find the closest point.
			lightCameraOrthographicMin = DirectX::XMVectorMin(tempTranslatedCornerPoint, lightCameraOrthographicMin);
			lightCameraOrthographicMax = DirectX::XMVectorMax(tempTranslatedCornerPoint, lightCameraOrthographicMax);
		}

		// This code removes the shimmering effect along the edges of shadows due to
		// the light changing to fit the camera.

		// We calculate a looser bound based on the size of the PCF blur.  This ensures us that we're 
		// sampling within the correct map.
		float scaleDueToBlurAMT = (float(3 * 2 + 1)
			/ float(shadowMapDimension));
		DirectX::XMVECTORF32 scaleDueToBlurAMTVec = { scaleDueToBlurAMT, scaleDueToBlurAMT, 0.0f, 0.0f };


		float normalizeByBufferSize = (1.0f / float(shadowMapDimension));
		DirectX::XMVECTOR normalizeByBufferSizeVec = DirectX::XMVectorSet(normalizeByBufferSize, normalizeByBufferSize, 0.0f, 0.0f);

		// We calculate the offsets as a percentage of the bound.
		DirectX::XMVECTOR boarderOffset = DirectX::XMVectorSubtract(lightCameraOrthographicMax, lightCameraOrthographicMin);
		boarderOffset = DirectX::XMVectorMultiply(boarderOffset, halfVector);
		boarderOffset = DirectX::XMVectorMultiply(boarderOffset, scaleDueToBlurAMTVec);
		lightCameraOrthographicMax = DirectX::XMVectorAdd(lightCameraOrthographicMax, boarderOffset);
		lightCameraOrthographicMin = DirectX::XMVectorSubtract(lightCameraOrthographicMin, boarderOffset);

		// The world units per texel are used to snap  the orthographic projection
		// to texel sized increments.  
		// Because we're fitting tighly to the cascades, the shimmering shadow edges will still be present when the 
		// camera rotates.  However, when zooming in or strafing the shadow edge will not shimmer.
		worldUnitsPerTexel = DirectX::XMVectorSubtract(lightCameraOrthographicMax, lightCameraOrthographicMin);
		worldUnitsPerTexel = DirectX::XMVectorMultiply(worldUnitsPerTexel, normalizeByBufferSizeVec);

		float lightCameraOrthographicMinZ = DirectX::XMVectorGetZ(lightCameraOrthographicMin);

		// We snap the camera to 1 pixel increments so that moving the camera does not cause the shadows to jitter.
		// This is a matter of integer dividing by the world space size of a texel
		lightCameraOrthographicMin = DirectX::XMVectorDivide(lightCameraOrthographicMin, worldUnitsPerTexel);
		lightCameraOrthographicMin = DirectX::XMVectorFloor(lightCameraOrthographicMin);
		lightCameraOrthographicMin = DirectX::XMVectorMultiply(lightCameraOrthographicMin, worldUnitsPerTexel);

		lightCameraOrthographicMax = DirectX::XMVectorDivide(lightCameraOrthographicMax, worldUnitsPerTexel);
		lightCameraOrthographicMax = DirectX::XMVectorFloor(lightCameraOrthographicMax);
		lightCameraOrthographicMax = DirectX::XMVectorMultiply(lightCameraOrthographicMax, worldUnitsPerTexel);


		// These are the unconfigured near and far plane values. They are purposely awful to show 
		// how important calculating accurate near and far planes is.
		float nearPlane = 0.0f;
		float farPlane = 10000.0f;

		DirectX::XMVECTOR lightSpaceSceneAABBminValue = FLTMAX;  // world space scene aabb 
		DirectX::XMVECTOR lightSpaceSceneAABBmaxValue = FLTMIN;
		// We calculate the min and max vectors of the scene in light space. The min and max "Z" values of the  
		// light space AABB can be used for the near and far plane. This is easier than intersecting the scene with the AABB
		// and in some cases provides similar results.
		for (int index = 0; index < 8; ++index)
		{
			lightSpaceSceneAABBminValue = DirectX::XMVectorMin(sceneAABBPointsLightSpace[index], lightSpaceSceneAABBminValue);
			lightSpaceSceneAABBmaxValue = DirectX::XMVectorMax(sceneAABBPointsLightSpace[index], lightSpaceSceneAABBmaxValue);
		}

		// The min and max z values are the near and far planes.
		nearPlane = DirectX::XMVectorGetZ(lightSpaceSceneAABBminValue);
		farPlane = DirectX::XMVectorGetZ(lightSpaceSceneAABBmaxValue);
		// Create the orthographic projection for this cascade.
		projection[cascadeIndex] = DirectX::XMMatrixOrthographicOffCenterLH(DirectX::XMVectorGetX(lightCameraOrthographicMin), DirectX::XMVectorGetX(lightCameraOrthographicMax),
			DirectX::XMVectorGetY(lightCameraOrthographicMin), DirectX::XMVectorGetY(lightCameraOrthographicMax),
			nearPlane, farPlane);
		projection[cascadeIndex] = DirectX::XMMatrixTranspose(projection[cascadeIndex]);
		cascadePartitionsFrustum[cascadeIndex] = frustumIntervalEnd;
	}
}

//--------------------------------------------------------------------------------------
// This function takes the camera's projection matrix and returns the 8
// points that make up a view frustum.
// The frustum is scaled to fit within the Begin and End interval paramaters.
//--------------------------------------------------------------------------------------
void Light::CreateFrustumPointsFromCascadeInterval(float fCascadeIntervalBegin,
	float fCascadeIntervalEnd,
	const DirectX::XMMATRIX& vProjection,
	DirectX::XMVECTOR* pvCornerPointsWorld) const
{
	DirectX::BoundingFrustum vViewFrust(vProjection);
	vViewFrust.Near = fCascadeIntervalBegin;
	vViewFrust.Far = fCascadeIntervalEnd;

	static const DirectX::XMVECTORU32 vGrabY = { 0x00000000,0xFFFFFFFF,0x00000000,0x00000000 };
	static const DirectX::XMVECTORU32 vGrabX = { 0xFFFFFFFF,0x00000000,0x00000000,0x00000000 };

	DirectX::XMVECTORF32 vRightTop = { vViewFrust.RightSlope,vViewFrust.TopSlope,1.0f,1.0f };
	DirectX::XMVECTORF32 vLeftBottom = { vViewFrust.LeftSlope,vViewFrust.BottomSlope,1.0f,1.0f };
	DirectX::XMVECTORF32 vNear = { vViewFrust.Near,vViewFrust.Near,vViewFrust.Near,1.0f };
	DirectX::XMVECTORF32 vFar = { vViewFrust.Far,vViewFrust.Far,vViewFrust.Far,1.0f };
	DirectX::XMVECTOR vRightTopNear = XMVectorMultiply(vRightTop, vNear);
	DirectX::XMVECTOR vRightTopFar = XMVectorMultiply(vRightTop, vFar);
	DirectX::XMVECTOR vLeftBottomNear = XMVectorMultiply(vLeftBottom, vNear);
	DirectX::XMVECTOR vLeftBottomFar = XMVectorMultiply(vLeftBottom, vFar);

	pvCornerPointsWorld[0] = vRightTopNear;
	pvCornerPointsWorld[1] = XMVectorSelect(vRightTopNear, vLeftBottomNear, vGrabX);
	pvCornerPointsWorld[2] = vLeftBottomNear;
	pvCornerPointsWorld[3] = XMVectorSelect(vRightTopNear, vLeftBottomNear, vGrabY);

	pvCornerPointsWorld[4] = vRightTopFar;
	pvCornerPointsWorld[5] = XMVectorSelect(vRightTopFar, vLeftBottomFar, vGrabX);
	pvCornerPointsWorld[6] = vLeftBottomFar;
	pvCornerPointsWorld[7] = XMVectorSelect(vRightTopFar, vLeftBottomFar, vGrabY);

}

LightStructure DirectionalLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 direction, float intensity, DirectX::XMFLOAT3 ambientColor)
{
	LightStructure result{};
	result.Color = color;
	result.AmbientColor = ambientColor;
	result.Direction = direction;
	result.Intensity = intensity;
	result.Type = LIGHT_TYPE_DIR;
	return result;
}

LightStructure PointLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 position, float range, float intensity)
{
	LightStructure result{};
	result.Color = color;
	result.Position = position;
	result.Range = range;
	result.Intensity = intensity;
	result.Type = LIGHT_TYPE_POINT;
	return result;
}

LightStructure SpotLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 direction, float range, float spotFalloff,
	float intensity)
{
	LightStructure result{};
	result.Color = color;
	result.Position = position;
	result.Direction = direction;
	result.Range = range;
	result.SpotFalloff = spotFalloff;
	result.Intensity = intensity;
	result.Type = LIGHT_TYPE_SPOT;
	return result;
}
