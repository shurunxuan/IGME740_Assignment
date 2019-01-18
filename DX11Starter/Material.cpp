#include "Material.h"


Material::Material(std::shared_ptr<SimpleVertexShader>& vtxShader, std::shared_ptr<SimplePixelShader>& pxlShader)
{
	vertexShader = vtxShader;
	pixelShader = pxlShader;

	printf("[INFO] Material created at <0x%p>.\n", this);
}

Material::~Material()
{
	printf("[INFO] Material destroyed at <0x%p>.\n", this);
}

SimpleVertexShader* Material::GetVertexShaderPtr()
{
	return vertexShader.get();
}

SimplePixelShader* Material::GetPixelShaderPtr()
{
	return pixelShader.get();
}
