#include "Material.h"

std::shared_ptr<Material> Material::defaultMaterial = std::make_unique<Material>();

Material::Material(ID3D11Device* d = nullptr)
{
	vertexShader = nullptr;
	pixelShader = nullptr;

	srvPtr = nullptr;
	samplerState = nullptr;

	device = d;

	printf("[INFO] Material created at <0x%p> by Material::Material(ID3D11Device* d).\n", this);
}


Material::Material(SimpleVertexShader* vtxShader, SimplePixelShader* pxlShader, ID3D11Device* d)
{
	vertexShader = vtxShader;
	pixelShader = pxlShader;

	srvPtr = nullptr;
	samplerState = nullptr;

	device = d;

	printf("[INFO] Material created at <0x%p> by Material::Material(SimpleVertexShader* vtxShader, SimplePixelShader* pxlShader, ID3D11Device* d).\n", this);
}

Material::~Material()
{
	if (samplerState) { samplerState->Release(); }
	if (srvPtr) { srvPtr->Release(); }

	printf("[INFO] Material destroyed at <0x%p>.\n", this);
}

SimpleVertexShader* Material::GetVertexShaderPtr() const
{
	return vertexShader;
}

SimplePixelShader* Material::GetPixelShaderPtr() const
{
	return pixelShader;
}

void Material::SetVertexShaderPtr(SimpleVertexShader* vShader)
{
	vertexShader = vShader;
}

void Material::SetPixelShaderPtr(SimplePixelShader* pShader)
{
	pixelShader = pShader;
}

ID3D11SamplerState* Material::GetSamplerState()
{
	return samplerState;
}

void Material::InitializeSampler()
{
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));

	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr = device->CreateSamplerState(&samplerDesc, &samplerState);
	if (FAILED(hr))
		printf("[WARNING] CreateSamplerState failed at <0x%p>.\n", this);
}

std::shared_ptr<Material> Material::GetDefault()
{
	return defaultMaterial;
}
