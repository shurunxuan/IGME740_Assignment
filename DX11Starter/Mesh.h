#pragma once

#include <memory>
#include <string>
#include <d3d11.h>
#include <vector>
#include "Vertex.h"
#include "Material.h"
#include "BlinnPhongMaterial.h"

class Mesh
{
public:
	Mesh(Vertex* vertices, int verticesCount, int* indices, int indicesCount, ID3D11Device* device);
	~Mesh();

	// Getters
	ID3D11Buffer* GetVertexBuffer() const { return vertexBuffer; }
	ID3D11Buffer* GetIndexBuffer() const { return indexBuffer; }
	int GetIndexCount() const { return indexCount; }
	Material* GetMaterial() const;

	void SetMaterial(std::shared_ptr<Material> m);

	static std::pair<std::vector<std::shared_ptr<Mesh>>, std::vector<std::shared_ptr<BlinnPhongMaterial>>> LoadFromFile(const std::string& filename, ID3D11Device* device, ID3D11DeviceContext* context);

private:
	// Buffers to hold actual geometry data
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;

	// Material
	std::shared_ptr<Material> material;

	int indexCount;
};

