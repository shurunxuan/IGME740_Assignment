#include "Light.h"
#include "SimpleLogger.h"

Light::Light(LightStructure* data, ID3D11Device* d, ID3D11DeviceContext* c)
{
	shadowMapDimension = 2048;
	shouldUpdateMatrices = true;
	Data = data;

	device = d;
	context = c;

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
	shadowMapDesc.Width = shadowMapDimension;

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
	shadowViewport = new D3D11_VIEWPORT;
	ZeroMemory(shadowViewport, sizeof(D3D11_VIEWPORT));
	shadowViewport->Height = float(shadowMapDimension);
	shadowViewport->Width = float(shadowMapDimension);
	shadowViewport->MinDepth = 0.f;
	shadowViewport->MaxDepth = 1.f;

	UpdateMatrices();

	LOG_INFO << "Light created at <0x" << this << ">." << std::endl;
}

Light::~Light()
{
	if (shadowMap) { shadowMap->Release(); }
	if (shadowDepthView) { shadowDepthView->Release(); }
	if (shadowResourceView) { shadowResourceView->Release(); }

	delete shadowViewport;

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

D3D11_VIEWPORT* Light::GetShadowViewport() const
{
	return shadowViewport;
}

DirectX::XMMATRIX Light::GetViewMatrix()
{
	if (shouldUpdateMatrices)
		UpdateMatrices();
	return view;
}

DirectX::XMMATRIX Light::GetProjectionMatrix()
{
	if (shouldUpdateMatrices)
		UpdateMatrices();
	return projection;
}

const LightStructure* Light::GetData() const
{
	return Data;
}

void Light::SetDirection(DirectX::XMFLOAT3 d)
{
	shouldUpdateMatrices = true;
	Data->Direction = d;
}

void Light::SetPosition(DirectX::XMFLOAT3 p)
{
	shouldUpdateMatrices = true;
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

void Light::SetRange(float r)
{
	shouldUpdateMatrices = true;
	Data->Range = r;
}

void Light::SetIntensity(float i) const
{
	Data->Intensity = i;
}

void Light::SetSpotFallOff(float s)
{
	shouldUpdateMatrices = true;
	Data->SpotFalloff = s;
}

void Light::UpdateMatrices()
{
	switch (Data->Type)
	{
	case LIGHT_TYPE_DIR:
	{
		const DirectX::XMVECTOR eyeDirection = DirectX::XMLoadFloat3(&Data->Direction);
		const DirectX::XMVECTOR eyePosition = DirectX::XMVectorSubtract(DirectX::XMVectorZero(), DirectX::XMVectorScale(eyeDirection, 50.0f));
		const DirectX::XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
		const DirectX::XMVECTOR upDirection = DirectX::XMLoadFloat3(&up);
		view = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookToLH(eyePosition, eyeDirection, upDirection));
		projection = XMMatrixTranspose(DirectX::XMMatrixOrthographicLH(10, 10, 0, 100));
	}
	break;
	case LIGHT_TYPE_POINT:

		break;
	case LIGHT_TYPE_SPOT:

		break;
	default:
		assert(true);
		break;
	}
	shouldUpdateMatrices = false;
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
