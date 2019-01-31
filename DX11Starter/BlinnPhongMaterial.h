#pragma once
#include "Material.h"
class BlinnPhongMaterial :
	public Material
{
public:
	BlinnPhongMaterial(ID3D11Device* d);
	using Material::Material;

	struct BlinnPhongMaterialStruct
	{
		// Some parameters for lighting
		DirectX::XMFLOAT4 ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 diffuse = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT4 specular = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT4 emission = { 0.0f, 0.0f, 0.0f, 1.0f };
		float shininess = 0.0f;
	} parameters;

	size_t GetMaterialStruct(void** mtlPtr) override;

	static std::shared_ptr<BlinnPhongMaterial> GetDefault();

private:
	static std::shared_ptr<BlinnPhongMaterial> defaultMaterial;

};

