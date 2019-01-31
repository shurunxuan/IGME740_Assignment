#include "BlinnPhongMaterial.h"

std::shared_ptr<BlinnPhongMaterial> BlinnPhongMaterial::defaultMaterial = std::make_unique<BlinnPhongMaterial>();

BlinnPhongMaterial::BlinnPhongMaterial(ID3D11Device* d = nullptr) : Material(d)
{
}

size_t BlinnPhongMaterial::GetMaterialStruct(void** mtlPtr)
{
	*mtlPtr = &parameters;
	return sizeof(BlinnPhongMaterialStruct);
}


std::shared_ptr<BlinnPhongMaterial> BlinnPhongMaterial::GetDefault()
{
	return defaultMaterial;
}
