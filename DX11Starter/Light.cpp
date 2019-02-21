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

	m_iCascadePartitionsZeroToOne[0] = 3;
	m_iCascadePartitionsZeroToOne[1] = 6;
	m_iCascadePartitionsZeroToOne[2] = 15;

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
	const DirectX::XMVECTOR g_vFLTMAX = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
	const DirectX::XMVECTOR g_vFLTMIN = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
	const DirectX::XMVECTOR g_vHalfVector = { 0.5f, 0.5f, 0.5f, 0.5f };

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
	DirectX::XMVECTOR vSceneAABBPointsLightSpace[8];
	for (int index = 0; index < 8; ++index)
	{
		DirectX::XMVECTOR v = XMLoadFloat3(&tmp[index]);
		vSceneAABBPointsLightSpace[index] = XMVector3Transform(v, matLightCameraView);
	}

	float fFrustumIntervalBegin, fFrustumIntervalEnd;
	DirectX::XMVECTOR vLightCameraOrthographicMin;  // light space frustum aabb 
	DirectX::XMVECTOR vLightCameraOrthographicMax;
	float fCameraNearFarRange = camera->GetFarClip() - camera->GetNearClip();

	DirectX::XMVECTOR vWorldUnitsPerTexel = { 0.0f, 0.0f, 0.0f, 0.0f };

	for (int iCascadeIndex = 0; iCascadeIndex < 3; ++iCascadeIndex)
	{
		// Calculate the interval of the View Frustum that this cascade covers. We measure the interval 
		// the cascade covers as a Min and Max distance along the Z Axis.

		// Because we want to fit the orthogrpahic projection tightly around the Cascade, we set the Mimiumum cascade 
		// value to the previous Frustum end Interval
		if (iCascadeIndex == 0) fFrustumIntervalBegin = 0.0f;
		else fFrustumIntervalBegin = float(m_iCascadePartitionsZeroToOne[iCascadeIndex - 1]);


		// Scale the intervals between 0 and 1. They are now percentages that we can scale with.
		fFrustumIntervalEnd = float(m_iCascadePartitionsZeroToOne[iCascadeIndex]);
		fFrustumIntervalBegin /= float(m_iCascadePartitionsMax);
		fFrustumIntervalEnd /= float(m_iCascadePartitionsMax);
		fFrustumIntervalBegin = fFrustumIntervalBegin * fCameraNearFarRange;
		fFrustumIntervalEnd = fFrustumIntervalEnd * fCameraNearFarRange;
		DirectX::XMVECTOR vFrustumPoints[8];

		// This function takes the began and end intervals along with the projection matrix and returns the 8
		// points that repreresent the cascade Interval
		CreateFrustumPointsFromCascadeInterval(fFrustumIntervalBegin, fFrustumIntervalEnd,
			matViewCameraProjection, vFrustumPoints);

		vLightCameraOrthographicMin = g_vFLTMAX;
		vLightCameraOrthographicMax = g_vFLTMIN;

		DirectX::XMVECTOR vTempTranslatedCornerPoint;
		// This next section of code calculates the min and max values for the orthographic projection.
		for (int icpIndex = 0; icpIndex < 8; ++icpIndex)
		{
			// Transform the frustum from camera view space to world space.
			vFrustumPoints[icpIndex] = XMVector4Transform(vFrustumPoints[icpIndex], matInverseViewCamera);
			// Transform the point from world space to Light Camera Space.
			vTempTranslatedCornerPoint = XMVector4Transform(vFrustumPoints[icpIndex], matLightCameraView);
			// Find the closest point.
			vLightCameraOrthographicMin = DirectX::XMVectorMin(vTempTranslatedCornerPoint, vLightCameraOrthographicMin);
			vLightCameraOrthographicMax = DirectX::XMVectorMax(vTempTranslatedCornerPoint, vLightCameraOrthographicMax);
		}

		// This code removes the shimmering effect along the edges of shadows due to
		// the light changing to fit the camera.

		// We calculate a looser bound based on the size of the PCF blur.  This ensures us that we're 
		// sampling within the correct map.
		float scaleDueToBlurAMT = (float(3 * 2 + 1)
			/ float(shadowMapDimension));
		DirectX::XMVECTORF32 scaleDueToBlurAMTVec = { scaleDueToBlurAMT, scaleDueToBlurAMT, 0.0f, 0.0f };


		float fNormalizeByBufferSize = (1.0f / float(shadowMapDimension));
		DirectX::XMVECTOR vNormalizeByBufferSize = DirectX::XMVectorSet(fNormalizeByBufferSize, fNormalizeByBufferSize, 0.0f, 0.0f);

		// We calculate the offsets as a percentage of the bound.
		DirectX::XMVECTOR vBoarderOffset = DirectX::XMVectorSubtract(vLightCameraOrthographicMax, vLightCameraOrthographicMin);
		vBoarderOffset = DirectX::XMVectorMultiply(vBoarderOffset, g_vHalfVector);
		vBoarderOffset = DirectX::XMVectorMultiply(vBoarderOffset, scaleDueToBlurAMTVec);
		vLightCameraOrthographicMax = DirectX::XMVectorAdd(vLightCameraOrthographicMax, vBoarderOffset);
		vLightCameraOrthographicMin = DirectX::XMVectorSubtract(vLightCameraOrthographicMin, vBoarderOffset);

		// The world units per texel are used to snap  the orthographic projection
		// to texel sized increments.  
		// Because we're fitting tighly to the cascades, the shimmering shadow edges will still be present when the 
		// camera rotates.  However, when zooming in or strafing the shadow edge will not shimmer.
		vWorldUnitsPerTexel = DirectX::XMVectorSubtract(vLightCameraOrthographicMax, vLightCameraOrthographicMin);
		vWorldUnitsPerTexel = DirectX::XMVectorMultiply(vWorldUnitsPerTexel, vNormalizeByBufferSize);

		float fLightCameraOrthographicMinZ = DirectX::XMVectorGetZ(vLightCameraOrthographicMin);

		// We snap the camera to 1 pixel increments so that moving the camera does not cause the shadows to jitter.
		// This is a matter of integer dividing by the world space size of a texel
		vLightCameraOrthographicMin = DirectX::XMVectorDivide(vLightCameraOrthographicMin, vWorldUnitsPerTexel);
		vLightCameraOrthographicMin = DirectX::XMVectorFloor(vLightCameraOrthographicMin);
		vLightCameraOrthographicMin = DirectX::XMVectorMultiply(vLightCameraOrthographicMin, vWorldUnitsPerTexel);

		vLightCameraOrthographicMax = DirectX::XMVectorDivide(vLightCameraOrthographicMax, vWorldUnitsPerTexel);
		vLightCameraOrthographicMax = DirectX::XMVectorFloor(vLightCameraOrthographicMax);
		vLightCameraOrthographicMax = DirectX::XMVectorMultiply(vLightCameraOrthographicMax, vWorldUnitsPerTexel);


		// These are the unconfigured near and far plane values. They are purposely awful to show 
		// how important calculating accurate near and far planes is.
		float fNearPlane = 0.0f;
		float fFarPlane = 10000.0f;

		DirectX::XMVECTOR vLightSpaceSceneAABBminValue = g_vFLTMAX;  // world space scene aabb 
		DirectX::XMVECTOR vLightSpaceSceneAABBmaxValue = g_vFLTMIN;
		// We calculate the min and max vectors of the scene in light space. The min and max "Z" values of the  
		// light space AABB can be used for the near and far plane. This is easier than intersecting the scene with the AABB
		// and in some cases provides similar results.
		for (int index = 0; index < 8; ++index)
		{
			vLightSpaceSceneAABBminValue = DirectX::XMVectorMin(vSceneAABBPointsLightSpace[index], vLightSpaceSceneAABBminValue);
			vLightSpaceSceneAABBmaxValue = DirectX::XMVectorMax(vSceneAABBPointsLightSpace[index], vLightSpaceSceneAABBmaxValue);
		}

		// The min and max z values are the near and far planes.
		fNearPlane = DirectX::XMVectorGetZ(vLightSpaceSceneAABBminValue);
		fFarPlane = DirectX::XMVectorGetZ(vLightSpaceSceneAABBmaxValue);
		// Create the orthographic projection for this cascade.
		projection[iCascadeIndex] = DirectX::XMMatrixOrthographicOffCenterLH(DirectX::XMVectorGetX(vLightCameraOrthographicMin), DirectX::XMVectorGetX(vLightCameraOrthographicMax),
			DirectX::XMVectorGetY(vLightCameraOrthographicMin), DirectX::XMVectorGetY(vLightCameraOrthographicMax),
			fNearPlane, fFarPlane);
		projection[iCascadeIndex] = DirectX::XMMatrixTranspose(projection[iCascadeIndex]);
		m_fCascadePartitionsFrustum[iCascadeIndex] = fFrustumIntervalEnd;
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
