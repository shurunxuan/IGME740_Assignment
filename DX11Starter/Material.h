#pragma once
#include <memory>
#include "SimpleShader.h"

// The class is not tested thoroughly and the resources are not managed gracefully.
class Material
{
public:
	Material(SimpleVertexShader* vtxShader, SimplePixelShader* pxlShader);
	~Material();

	SimpleVertexShader* GetVertexShaderPtr() const;
	SimplePixelShader* GetPixelShaderPtr() const;

private:
	// Wrappers for DirectX shaders to provide simplified functionality
	SimpleVertexShader* vertexShader;
	SimplePixelShader* pixelShader;
};

