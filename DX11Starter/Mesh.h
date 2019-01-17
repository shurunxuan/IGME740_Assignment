#pragma once

#include <d3d11.h>
#include "Vertex.h"

class Mesh
{
public:
	Mesh(Vertex* vertices, int verticesCount, int* indices, int indicesCount, ID3D11Device* device);
	~Mesh();

	// Getters
	ID3D11Buffer* GetVertexBuffer() const { return vertexBuffer; }
	ID3D11Buffer* GetIndexBuffer() const { return indexBuffer; }
	int GetIndexCount() const { return indexCount; }

private:
	// Buffers to hold actual geometry data
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;

	int indexCount;
};

