#pragma once
#include <memory>
#include "SimpleShader.h"

// The class is not tested thoroughly and the resources are not managed gracefully.
class Material
{
public:
	Material(std::shared_ptr<SimpleVertexShader>& vtxShader, std::shared_ptr<SimplePixelShader>& pxlShader);
	~Material();

	SimpleVertexShader* GetVertexShaderPtr();
	SimplePixelShader* GetPixelShaderPtr();

private:
	// Wrappers for DirectX shaders to provide simplified functionality
	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> pixelShader;
};

