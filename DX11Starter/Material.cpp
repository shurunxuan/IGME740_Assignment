#include "Material.h"


Material::Material(std::shared_ptr<SimpleVertexShader>& vtxShader, std::shared_ptr<SimplePixelShader>& pxlShader)
{
	vertexShader = vtxShader;
	pixelShader = pxlShader;
}

Material::~Material()
{
}

SimpleVertexShader* Material::GetVertexShaderPtr()
{
	return vertexShader.get();
}

SimplePixelShader* Material::GetPixelShaderPtr()
{
	return pixelShader.get();
}
