#pragma once

#include <memory>
#include <string>
#include <d3d11.h>
#include <vector>
#include "Vertex.h"
#include "SimpleShader.h"

class Skybox
{
public:
	Skybox(ID3D11Device* d, ID3D11DeviceContext* c, const std::string& cubemapFile);
	~Skybox();

	ID3D11Buffer* GetVertexBuffer() const;
	ID3D11Buffer* GetIndexBuffer() const;

	SimpleVertexShader* GetVertexShader() const;
	SimplePixelShader* GetPixelShader() const;

	void SetVertexShader(SimpleVertexShader* vShader);
	void SetPixelShader(SimplePixelShader* pShader);

	ID3D11SamplerState* GetSamplerState() const;

	ID3D11ShaderResourceView* GetCubemapSrv() const;
private:
	ID3D11Device* device; 
	ID3D11DeviceContext* context;

	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;

	SimpleVertexShader* vertexShader;
	SimplePixelShader* pixelShader;

	D3D11_SAMPLER_DESC samplerDesc;
	ID3D11SamplerState* samplerState;

	ID3D11Resource* cubemapTex;
	ID3D11ShaderResourceView* cubemapSrv;
};

