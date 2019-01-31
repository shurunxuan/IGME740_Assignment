#include "BrdfMaterial.h"

std::shared_ptr<BrdfMaterial> BrdfMaterial::defaultMaterial = std::make_unique<BrdfMaterial>();

BrdfMaterial::BrdfMaterial(ID3D11Device* d = nullptr) : Material(d)
{
}

size_t BrdfMaterial::GetMaterialStruct(void** mtlPtr)
{
	*mtlPtr = &parameters;
	return sizeof(BrdfMaterialStruct);
}


std::shared_ptr<BrdfMaterial> BrdfMaterial::GetDefault()
{
	return defaultMaterial;
}