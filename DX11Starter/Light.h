#pragma once
#include <DirectXMath.h>
#include <WICTextureLoader.h>

#define LIGHT_TYPE_DIR 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

struct LightStructure
{
	int Type;
	DirectX::XMFLOAT3 Direction;// 16 bytes
	float Range;
	DirectX::XMFLOAT3 Position;// 32 bytes
	float Intensity;
	DirectX::XMFLOAT3 Color;// 48 bytes
	float SpotFalloff;
	DirectX::XMFLOAT3 AmbientColor;// 64 bytes
};

class Light
{
public:
	Light(LightStructure* data, ID3D11Device* d, ID3D11DeviceContext* c);
	~Light();


	ID3D11Texture2D* GetShadowMap() const;
	ID3D11DepthStencilView* GetShadowDepthView() const;
	ID3D11ShaderResourceView* GetShadowResourceView() const;

	D3D11_VIEWPORT* GetShadowViewport() const;

	DirectX::XMMATRIX GetViewMatrix();
	DirectX::XMMATRIX GetProjectionMatrix();

	const LightStructure* GetData() const;

	void SetDirection(DirectX::XMFLOAT3 d);
	void SetPosition(DirectX::XMFLOAT3 p);
	void SetColor(DirectX::XMFLOAT3 c) const;
	void SetAmbientColor(DirectX::XMFLOAT3 a) const;
	void SetRange(float r);
	void SetIntensity(float i) const;
	void SetSpotFallOff(float s);
private:
	UINT shadowMapDimension;

	ID3D11Device* device;
	ID3D11DeviceContext* context;

	ID3D11Texture2D* shadowMap;
	ID3D11DepthStencilView* shadowDepthView;
	ID3D11ShaderResourceView* shadowResourceView;

	D3D11_VIEWPORT* shadowViewport;

	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;

	bool shouldUpdateMatrices;

	LightStructure* Data;

	void UpdateMatrices();
};

LightStructure DirectionalLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 direction, float intensity, DirectX::XMFLOAT3 ambientColor);
LightStructure PointLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 position, float range, float intensity);
LightStructure SpotLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 direction, float range, float spotFalloff, float intensity);