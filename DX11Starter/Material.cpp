#include "Material.h"

std::shared_ptr<Material> Material::defaultMaterial = std::make_unique<Material>();

Material::Material()
{
	vertexShader = nullptr;
	pixelShader = nullptr;

	printf("[INFO] Material created at <0x%p> by Material::Material().\n", this);
}


Material::Material(SimpleVertexShader* vtxShader, SimplePixelShader* pxlShader)
{
	vertexShader = vtxShader;
	pixelShader = pxlShader;

	printf("[INFO] Material created at <0x%p> by Material::Material(SimpleVertexShader* vtxShader, SimplePixelShader* pxlShader).\n", this);
}

Material::~Material()
{
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

std::shared_ptr<Material> Material::GetDefault()
{
	return defaultMaterial;
}
