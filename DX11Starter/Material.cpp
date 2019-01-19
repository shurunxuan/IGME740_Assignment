#include "Material.h"


Material::Material(SimpleVertexShader* vtxShader, SimplePixelShader* pxlShader)
{
	vertexShader = vtxShader;
	pixelShader = pxlShader;

	printf("[INFO] Material created at <0x%p>.\n", this);
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
