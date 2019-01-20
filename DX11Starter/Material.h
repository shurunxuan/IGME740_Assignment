#pragma once
#include "SimpleShader.h"
#include <memory>

// The class is not tested thoroughly and the resources are not managed gracefully.
class Material
{
public:
	Material(ID3D11Device* d);
	Material(SimpleVertexShader* vtxShader, SimplePixelShader* pxlShader, ID3D11Device* d);
	~Material();

	// Getters
	SimpleVertexShader* GetVertexShaderPtr() const;
	SimplePixelShader* GetPixelShaderPtr() const;
	// Setters
	void SetVertexShaderPtr(SimpleVertexShader* vShader);
	void SetPixelShaderPtr(SimplePixelShader* pShader);

	ID3D11SamplerState* GetSamplerState();

	void InitializeSampler();

	static std::shared_ptr<Material> GetDefault();

	// Some parameters for lighting
	DirectX::XMFLOAT3 ambient = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 diffuse = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 specular = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 emission = { 0.0f, 0.0f, 0.0f };
	float shininess = 0.0f;
	// texture
	ID3D11ShaderResourceView* srvPtr;

private:
	// Wrappers for DirectX shaders to provide simplified functionality
	// Material class doesn't maintain the life cycle of shader pointers
	SimpleVertexShader* vertexShader;
	SimplePixelShader* pixelShader;

	ID3D11Device* device;

	D3D11_SAMPLER_DESC samplerDesc;
	ID3D11SamplerState* samplerState;

	static std::shared_ptr<Material> defaultMaterial;
};

