#include <cstdio>
#include <map>
#include <sstream>
#include <fstream>
#include <utility>
#include <locale>
#include <codecvt>
#include <DirectXMath.h>
#include <WICTextureLoader.h>
#include "Mesh.h"


Mesh::Mesh(Vertex* vertices, int verticesCount, int* indices, int indicesCount, ID3D11Device* device)
{
	indexCount = indicesCount;
	material = nullptr;

	// Create the VERTEX BUFFER description -----------------------------------
	// - The description is created on the stack because we only need
	//    it to create the buffer.  The description is then useless.
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * verticesCount;       // 3 = number of vertices in the buffer
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER; // Tells DirectX this is a vertex buffer
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	// Create the proper struct to hold the initial vertex data
	// - This is how we put the initial data into the buffer
	D3D11_SUBRESOURCE_DATA initialVertexData;
	initialVertexData.pSysMem = vertices;

	// Actually create the buffer with the initial data
	// - Once we do this, we'll NEVER CHANGE THE BUFFER AGAIN
	if (device)
		device->CreateBuffer(&vbd, &initialVertexData, &vertexBuffer);
	else
	{
		printf("[ERROR] Error when creating vertex buffer: ID3D11Device is null.\n");
		system("pause");
		exit(-1);
	}

	// Create the INDEX BUFFER description ------------------------------------
	// - The description is created on the stack because we only need
	//    it to create the buffer.  The description is then useless.
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(int) * indexCount;         // 3 = number of indices in the buffer
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER; // Tells DirectX this is an index buffer
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	// Create the proper struct to hold the initial index data
	// - This is how we put the initial data into the buffer
	D3D11_SUBRESOURCE_DATA initialIndexData;
	initialIndexData.pSysMem = indices;

	// Actually create the buffer with the initial data
	// - Once we do this, we'll NEVER CHANGE THE BUFFER AGAIN
	device->CreateBuffer(&ibd, &initialIndexData, &indexBuffer);

	printf("[INFO] Mesh created at <0x%p>.\n", this);
}


Mesh::~Mesh()
{
	if (vertexBuffer) { vertexBuffer->Release(); }
	if (indexBuffer) { indexBuffer->Release(); }

	printf("[INFO] Mesh destroyed at <0x%p>.\n", this);
}

std::vector<std::string> split(std::string str, char ch)
{
	std::vector<std::string> result;
	std::string temp(std::move(str));
	size_t pos;

	while ((pos = temp.find(ch)) != std::string::npos)
	{
		std::string s(temp.begin(), temp.begin() + pos);
		result.push_back(s);
		temp = std::string(temp.begin() + pos + 1, temp.end());
	}
	result.push_back(temp);

	return result;
}

template <typename T>
T str2num(const std::string& str, T& num)
{
	std::stringstream ss;
	ss << str;
	ss >> num;
	return num;
}

Material* Mesh::GetMaterial() const
{
	if (material == nullptr)
	{
		printf("[WARNING] Material of Mesh <0x%p> is not set! Fallback to default material.\n", this);
		return Material::GetDefault().get();
	}

	return material.get();
}

void Mesh::SetMaterial(std::shared_ptr<Material> m)
{
	material = std::move(m);
}

std::pair<std::vector<std::shared_ptr<Mesh>>, std::vector<std::shared_ptr<Material>>> Mesh::LoadFromFile(const std::string& filename, ID3D11Device* device, ID3D11DeviceContext* context)
{
	std::vector<std::shared_ptr<Mesh>> meshList;
	std::vector<std::shared_ptr<Material>> materialList;
	std::map<std::string, std::shared_ptr<Material>> materialMap;

	std::vector<DirectX::XMFLOAT3> positions;
	std::vector<DirectX::XMFLOAT3> normals;
	std::vector<DirectX::XMFLOAT2> texcoords;

	std::vector<std::vector<int>> indices;
	std::vector<std::vector<int>> vertices;

	std::vector<std::string> mtlOfMeshes;

	int* indexBuffer = nullptr;
	Vertex* vertexBuffer = nullptr;

	std::string currentMtl;

	materialMap[""] = Material::GetDefault();

	// Read .obj file
	std::ifstream fin(filename);
	printf("[INFO] OBJ file \"%s\" opened.\n", filename.c_str());
	std::string line;
	std::string mtlFile;
	std::string folder;
	while (getline(fin, line))
	{
		auto s = split(line, ' ');
		std::string first_token(s[0]);
		if (first_token == "mtllib")
		{
			auto base = split(filename, '\\');
			mtlFile = "";
			for (unsigned i = 0; i != base.size() - 1; ++i)
			{
				folder += base[i] + "\\";
			}
			mtlFile = folder + s[1];
		}
		if (first_token == "v")
		{
			DirectX::XMFLOAT3 v = {};
			str2num(s[1], v.x);
			str2num(s[2], v.y);
			str2num(s[3], v.z);
			v.z *= -1.0f;
			positions.push_back(v);

		}
		else if (first_token == "vn")
		{
			DirectX::XMFLOAT3 n = {};
			str2num(s[1], n.x);
			str2num(s[2], n.y);
			str2num(s[3], n.z);
			n.z *= -1.0f;
			normals.push_back(n);
		}
		else if (first_token == "vt")
		{
			DirectX::XMFLOAT2 t = {};
			str2num(s[1], t.x);
			str2num(s[2], t.y);
			t.y = 1.0f - t.y;
			texcoords.push_back(t);
		}
		else if (first_token == "f")
		{
			int index[4];
			int vtxV[4];
			int vtxT[4];
			int vtxN[4];
			bool hasNormal = true;
			for (unsigned i = 0; i != s.size() - 1; ++i)
			{
				std::string p(s[i + 1]);
				auto p_detail = split(p, '/');
				int v = -1, n = -1, t = -1;
				str2num(p_detail[0], vtxV[i]);
				str2num(p_detail[1], vtxT[i]);
				if (p_detail.size() > 2)
					str2num(p_detail[2], vtxN[i]);
				else
					hasNormal = false;
			}
			// No normal data, generate it
			if (!hasNormal)
			{
				DirectX::XMFLOAT3 pos0 = positions[vtxV[0] - 1];
				DirectX::XMFLOAT3 pos1 = positions[vtxV[1] - 1];
				DirectX::XMFLOAT3 pos2 = positions[vtxV[2] - 1];

				DirectX::XMVECTOR posV0 = DirectX::XMLoadFloat3(&pos0);
				DirectX::XMVECTOR posV1 = DirectX::XMLoadFloat3(&pos1);
				DirectX::XMVECTOR posV2 = DirectX::XMLoadFloat3(&pos2);

				DirectX::XMVECTOR v1 = DirectX::XMVectorSubtract(posV1, posV0);
				DirectX::XMVECTOR v2 = DirectX::XMVectorSubtract(posV1, posV2);

				DirectX::XMVECTOR nV = DirectX::XMVector3Cross(v1, v2);

				DirectX::XMFLOAT3 n{};
				DirectX::XMStoreFloat3(&n, nV);

				normals.push_back(n);
				vtxN[0] = vtxN[1] = vtxN[2] = int(normals.size());
				if (s.size() == 5) vtxN[3] = vtxN[0];
			}

			for (unsigned i = 0; i != s.size() - 1; ++i)
			{
				std::vector<int> vertexData = { vtxV[i], vtxN[i], vtxT[i] };

				unsigned findResult = unsigned(std::find(vertices.begin(), vertices.end(), vertexData) - vertices.begin());
				if (findResult == vertices.size())
				{
					vertices.push_back(vertexData);
				}
				index[i] = findResult;
			}
			indices.push_back({ index[0], index[2], index[1] });

			if (s.size() == 5)
			{
				// 4th face
				indices.push_back({ index[0], index[3], index[2] });
			}
		}
		else if (first_token == "usemtl")
		{
			if (!currentMtl.empty())
			{
				// Create new mesh
				indexBuffer = new int[indices.size() * 3];
				vertexBuffer = new Vertex[vertices.size()];
				// Generate indexBuffer
				for (unsigned i = 0; i != indices.size(); ++i)
				{
					for (unsigned j = 0; j < 3; ++j)
					{
						indexBuffer[i * 3 + j] = indices[i][j];
					}
				}
				// Generate vertexBuffer
				int i = 0;
				for (auto& it : vertices)
				{
					// No normal data
					DirectX::XMFLOAT3 zero = { 0.0f, 0.0f, 0.0f };
					if (it[1] < 0)
					{
						Vertex vtx{ positions[it[0] - 1], zero, texcoords[it[2] - 1] };
						vertexBuffer[i] = vtx;
					}
					else
					{
						Vertex vtx{ positions[it[0] - 1], normals[it[1] - 1], texcoords[it[2] - 1] };
						vertexBuffer[i] = vtx;
					}
					
					++i;
				}
				std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>(vertexBuffer, int(vertices.size()), indexBuffer, int(indices.size()) * 3, device);
				meshList.push_back(newMesh);
				mtlOfMeshes.push_back(currentMtl);

				delete[] indexBuffer;
				delete[] vertexBuffer;

				indices.clear();
				vertices.clear();
			}

			currentMtl = s[1];
		}
	}

	// Create new mesh
	indexBuffer = new int[indices.size() * 3];
	vertexBuffer = new Vertex[vertices.size()];
	// Generate indexBuffer
	for (unsigned i = 0; i != indices.size(); ++i)
	{
		for (unsigned j = 0; j < 3; ++j)
		{
			indexBuffer[i * 3 + j] = indices[i][j];
		}
	}
	// Generate vertexBuffer
	int i = 0;
	for (auto& it : vertices)
	{
		// No normal data
		DirectX::XMFLOAT3 zero = { 0.0f, 0.0f, 0.0f };
		if (it[1] < 0)
		{
			Vertex vtx{ positions[it[0] - 1], zero, texcoords[it[2] - 1] };
			vertexBuffer[i] = vtx;
		}
		else
		{
			Vertex vtx{ positions[it[0] - 1], normals[it[1] - 1], texcoords[it[2] - 1] };
			vertexBuffer[i] = vtx;
		}
		++i;
	}
	std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>(vertexBuffer, int(vertices.size()), indexBuffer, int(indices.size() * 3), device);
	meshList.push_back(newMesh);
	mtlOfMeshes.push_back(currentMtl);

	delete[] indexBuffer;
	delete[] vertexBuffer;

	indices.clear();
	vertices.clear();

	// Read .mtl file
	if (!mtlFile.empty())
	{
		std::ifstream mltFin(mtlFile);
		printf("[INFO] MTL file \"%s\" opened.\n", mtlFile.c_str());
		std::string mtlLine;
		std::shared_ptr<Material> current_mtl = nullptr;
		std::string currentName;
		while (getline(mltFin, mtlLine))
		{
			auto s = split(mtlLine, ' ');
			std::string first_token(s[0]);
			if (first_token == "newmtl")
			{
				if (current_mtl != nullptr)
				{
					// save last material
					materialList.push_back(current_mtl);
					materialMap[currentName] = current_mtl;
				}

				currentName = s[1];
				current_mtl = std::make_shared<Material>(device);
			}
			else if (first_token == "Kd")
			{
				float r, g, b;
				str2num(s[1], r);
				str2num(s[2], g);
				str2num(s[3], b);
				current_mtl->diffuse = DirectX::XMFLOAT4(r, g, b, 1.0f);
			}
			else if (first_token == "Ka")
			{
				float r, g, b;
				str2num(s[1], r);
				str2num(s[2], g);
				str2num(s[3], b);
				current_mtl->ambient = DirectX::XMFLOAT4(r, g, b, 1.0f);
			}
			else if (first_token == "Ks")
			{
				float r, g, b;
				str2num(s[1], r);
				str2num(s[2], g);
				str2num(s[3], b);
				current_mtl->specular = DirectX::XMFLOAT4(r, g, b, 1.0f);
			}
			else if (first_token == "Ke")
			{
				float r, g, b;
				str2num(s[1], r);
				str2num(s[2], g);
				str2num(s[3], b);
				current_mtl->emission = DirectX::XMFLOAT4(r, g, b, 1.0f);
			}
			else if (first_token == "Ns")
			{
				//current_mtl->shininess = 20;
				str2num(s[1], current_mtl->shininess);
			}
			else if (first_token == "map_Kd")
			{
				// texture file
				std::string name = folder + s[1];
				// string -> wstring
				std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
				std::wstring wName = cv.from_bytes(name);

				HRESULT hr = DirectX::CreateWICTextureFromFile(device, context, wName.c_str(), nullptr, &current_mtl->srvPtr);
				if (FAILED(hr))
				{
					printf("[WARNING] Failed to load texture file \"%s\".\n", name.c_str());
				}
				else
				{
					printf("[INFO] Load texture file \"%s\".\n", name.c_str());
					current_mtl->InitializeSampler();
				}
			}
		}

		// save last material
		materialList.push_back(current_mtl);
		materialMap[currentName] = current_mtl;
	}
	else
	{
		printf("[INFO] No mtl data in file \"%s\" found. Fallback to default material.\n", filename.c_str());
		materialList.push_back(Material::GetDefault());
	}

	// Set Material of Mesh
	for (unsigned m = 0; m != meshList.size(); ++m)
	{
		meshList[m]->SetMaterial(materialMap[mtlOfMeshes[m]]);
	}


	std::pair<std::vector<std::shared_ptr<Mesh>>, std::vector<std::shared_ptr<Material>>> result(meshList, materialList);
	return result;
}
