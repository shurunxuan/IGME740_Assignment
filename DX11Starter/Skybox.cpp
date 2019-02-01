#include "Skybox.h"
#include <DDSTextureLoader.h>
#include "SimpleLogger.h"
#include <locale>
#include <codecvt>

Skybox::Skybox(ID3D11Device* d, ID3D11DeviceContext* c, const std::string& cubemapFile, const std::string& irradianceFile)
{
	device = d;
	context = c;

	rotation = DirectX::XMQuaternionIdentity();

	// string -> wstring
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
	std::wstring wFilename = cv.from_bytes(cubemapFile);
	// Load cubemap
	DirectX::CreateDDSTextureFromFile(device, context, wFilename.c_str(), &cubemapTex, &cubemapSrv);

	wFilename = cv.from_bytes(irradianceFile);
	// Load irradiance map
	DirectX::CreateDDSTextureFromFile(device, context, wFilename.c_str(), &irradianceTex, &irradianceSrv);

	Vertex vertices[8];

	vertices[0].Position = DirectX::XMFLOAT3(-0.5f, +0.5f, -0.5f);
	vertices[1].Position = DirectX::XMFLOAT3(+0.5f, +0.5f, -0.5f);
	vertices[2].Position = DirectX::XMFLOAT3(+0.5f, +0.5f, +0.5f);
	vertices[3].Position = DirectX::XMFLOAT3(-0.5f, +0.5f, +0.5f);
	vertices[4].Position = DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f);
	vertices[5].Position = DirectX::XMFLOAT3(+0.5f, -0.5f, -0.5f);
	vertices[6].Position = DirectX::XMFLOAT3(+0.5f, -0.5f, +0.5f);
	vertices[7].Position = DirectX::XMFLOAT3(-0.5f, -0.5f, +0.5f);

	int indices[36] =
	{
		0, 2, 3, 0, 1, 2,
		0, 4, 1, 4, 5, 1,
		1, 5, 6, 1, 6, 2,
		7, 3, 2, 7, 2, 6,
		0, 3, 4, 4, 3, 7,
		4, 7, 5, 5, 7, 6
	};

	// Create the VERTEX BUFFER description -----------------------------------
	// - The description is created on the stack because we only need
	//    it to create the buffer.  The description is then useless.
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * 8;       // 3 = number of vertices in the buffer
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER; // Tells DirectX this is a vertex buffer
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	// Create the proper struct to hold the initial vertex data
	// - This is how we put the initial data into the buffer
	D3D11_SUBRESOURCE_DATA initialVertexData;
	initialVertexData.pSysMem = vertices;

	// Actually create the buffer with the initial data
	// - Once we do this, we'll NEVER CHANGE THE BUFFER AGAIN
	if (device)
		device->CreateBuffer(&vbd, &initialVertexData, &vertexBuffer);
	else
	{
		LOG_ERROR << "Error when creating vertex buffer for skybox: ID3D11Device is null." << std::endl;
		system("pause");
		exit(-1);
	}

	// Create the INDEX BUFFER description ------------------------------------
	// - The description is created on the stack because we only need
	//    it to create the buffer.  The description is then useless.
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(int) * 36;         // 3 = number of indices in the buffer
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER; // Tells DirectX this is an index buffer
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	// Create the proper struct to hold the initial index data
	// - This is how we put the initial data into the buffer
	D3D11_SUBRESOURCE_DATA initialIndexData;
	initialIndexData.pSysMem = indices;

	// Actually create the buffer with the initial data
	// - Once we do this, we'll NEVER CHANGE THE BUFFER AGAIN
	device->CreateBuffer(&ibd, &initialIndexData, &indexBuffer);

	ZeroMemory(&samplerDesc, sizeof(samplerDesc));

	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr = device->CreateSamplerState(&samplerDesc, &samplerState);
	if (FAILED(hr))
		LOG_ERROR << "CreateSamplerState failed at <0x" << this << ">." << std::endl;

	LOG_INFO << "Skybox created at 0x<" << this << "> with file \"" << cubemapFile << "\"." << std::endl;

}

Skybox::~Skybox()
{
	if (vertexBuffer) { vertexBuffer->Release(); }
	if (indexBuffer) { indexBuffer->Release(); }

	if (samplerState) { samplerState->Release(); }

	if (cubemapTex) { cubemapTex->Release(); }
	if (cubemapSrv) { cubemapSrv->Release(); }	
	if (irradianceTex) { irradianceTex->Release(); }
	if (irradianceSrv) { irradianceSrv->Release(); }

	LOG_INFO << "Skybox destroyed at 0x<" << this << ">." << std::endl;
}

ID3D11Buffer* Skybox::GetVertexBuffer() const
{
	return vertexBuffer;
}

ID3D11Buffer* Skybox::GetIndexBuffer() const
{
	return indexBuffer;
}

SimpleVertexShader* Skybox::GetVertexShader() const
{
	return vertexShader;
}

SimplePixelShader* Skybox::GetPixelShader() const
{
	return pixelShader;
}

void Skybox::SetVertexShader(SimpleVertexShader* vShader)
{
	vertexShader = vShader;
}

void Skybox::SetPixelShader(SimplePixelShader* pShader)
{
	pixelShader = pShader;
}

ID3D11SamplerState* Skybox::GetSamplerState() const
{
	return samplerState;
}

ID3D11ShaderResourceView* Skybox::GetCubemapSrv() const
{
	return cubemapSrv;
}

ID3D11ShaderResourceView* Skybox::GetIrradianceSrv() const
{
	return irradianceSrv;
}

DirectX::XMVECTOR Skybox::GetRotationQuaternion() const
{
	return rotation;
}

void Skybox::SetRotationQuaternion(DirectX::XMVECTOR& r)
{
	rotation = r;
}
