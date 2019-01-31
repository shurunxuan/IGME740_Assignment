#pragma once
#include "Material.h"
class BrdfMaterial :
	public Material
{
public:
	BrdfMaterial(ID3D11Device* d);
	using Material::Material;

	struct BrdfMaterialStruct
	{
		// Some parameters for lighting
		/*DirectX::XMFLOAT4 ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 diffuse = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT4 specular = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT4 emission = { 0.0f, 0.0f, 0.0f, 1.0f };
		float shininess = 0.0f;*/
		// F_0
		DirectX::XMFLOAT3 reflectance = { 1.0f, 1.0f, 1.0f };
		float roughness = 0.5f;
		float metalness = 0.5f;
	} parameters;

	size_t GetMaterialStruct(void** mtlPtr) override;

	static std::shared_ptr<BrdfMaterial> GetDefault();

private:
	static std::shared_ptr<BrdfMaterial> defaultMaterial;
};

