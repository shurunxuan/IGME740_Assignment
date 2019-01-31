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
#include "SimpleLogger.h"


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
		LOG_ERROR << "Error when creating vertex buffer: ID3D11Device is null." << std::endl;
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

	LOG_INFO << "Mesh created at <0x" << this << "> by " << __FUNCTION__ << "." << std::endl;
}


Mesh::~Mesh()
{
	if (vertexBuffer) { vertexBuffer->Release(); }
	if (indexBuffer) { indexBuffer->Release(); }

	LOG_INFO << "Mesh destroyed at <0x" << this << ">." << std::endl;
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
T str2num(const std::string & str, T & num)
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
		LOG_WARNING << "Material of Mesh <0x" << this << "> is not set! Fallback to default material." << std::endl;
		return reinterpret_cast<Material*>(BlinnPhongMaterial::GetDefault().get());
	}

	return material.get();
}

void Mesh::SetMaterial(std::shared_ptr<Material> m)
{
	material = std::move(m);
}

std::pair<std::vector<std::shared_ptr<Mesh>>, std::vector<std::shared_ptr<BlinnPhongMaterial>>> Mesh::LoadFromFile(const std::string & filename, ID3D11Device * device, ID3D11DeviceContext * context)
{
	std::vector<std::shared_ptr<Mesh>> meshList;
	std::vector<std::shared_ptr<BlinnPhongMaterial>> materialList;
	std::map<std::string, std::shared_ptr<BlinnPhongMaterial>> materialMap;

	std::vector<DirectX::XMFLOAT3> positions;
	std::vector<DirectX::XMFLOAT3> normals;
	std::vector<DirectX::XMFLOAT2> texcoords;

	std::vector<DirectX::XMVECTOR> tangentsPerPositions;

	std::vector<std::vector<int>> indices;
	std::vector<std::vector<int>> vertices;

	std::vector<std::string> mtlOfMeshes;

	int* indexBuffer = nullptr;
	Vertex* vertexBuffer = nullptr;

	std::string currentMtl;

	materialMap[""] = BlinnPhongMaterial::GetDefault();

	// Read .obj file
	std::ifstream fin(filename);
	LOG_INFO << "OBJ file \"" << filename << "\" opened." << std::endl;
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
			tangentsPerPositions.resize(positions.size(), DirectX::XMVectorZero());
			int index[3];
			int vtxV[3];
			int vtxT[3];
			int vtxN[3];
			int vtxTan[3];
			int vtxBit[3];
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
				if (normals.size() < positions.size())
					normals.resize(positions.size(), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));
				DirectX::XMFLOAT3 pos0 = positions[vtxV[0] - 1];
				DirectX::XMFLOAT3 pos1 = positions[vtxV[1] - 1];
				DirectX::XMFLOAT3 pos2 = positions[vtxV[2] - 1];

				DirectX::XMVECTOR posV0 = XMLoadFloat3(&pos0);
				DirectX::XMVECTOR posV1 = XMLoadFloat3(&pos1);
				DirectX::XMVECTOR posV2 = XMLoadFloat3(&pos2);

				DirectX::XMVECTOR v1 = DirectX::XMVectorSubtract(posV1, posV0);
				DirectX::XMVECTOR v2 = DirectX::XMVectorSubtract(posV1, posV2);

				DirectX::XMVECTOR nV = DirectX::XMVector3Cross(v1, v2);

				DirectX::XMFLOAT3 n{};
				XMStoreFloat3(&n, nV);

				DirectX::XMVECTOR n0 = XMLoadFloat3(&normals[vtxV[0] - 1]);
				DirectX::XMVECTOR n1 = XMLoadFloat3(&normals[vtxV[1] - 1]);
				DirectX::XMVECTOR n2 = XMLoadFloat3(&normals[vtxV[2] - 1]);

				n0 = DirectX::XMVectorAdd(n0, nV);
				n1 = DirectX::XMVectorAdd(n1, nV);
				n2 = DirectX::XMVectorAdd(n2, nV);

				XMStoreFloat3(&normals[vtxV[0] - 1], n0);
				XMStoreFloat3(&normals[vtxV[1] - 1], n1);
				XMStoreFloat3(&normals[vtxV[2] - 1], n2);

				vtxN[0] = vtxV[0];
				vtxN[1] = vtxV[1];
				vtxN[2] = vtxV[2];
			}

			// Now to calculate tangent and bitangent
			// We work relative to v0
			DirectX::XMVECTOR P0 = XMLoadFloat3(&positions[vtxV[0] - 1]);
			DirectX::XMVECTOR P1 = XMLoadFloat3(&positions[vtxV[1] - 1]);
			DirectX::XMVECTOR P2 = XMLoadFloat3(&positions[vtxV[2] - 1]);

			DirectX::XMVECTOR Q1V = DirectX::XMVectorSubtract(P1, P0);
			DirectX::XMVECTOR Q2V = DirectX::XMVectorSubtract(P2, P0);

			DirectX::XMFLOAT3 Q1{};
			DirectX::XMFLOAT3 Q2{};
			XMStoreFloat3(&Q1, Q1V);
			XMStoreFloat3(&Q2, Q2V);

			DirectX::XMFLOAT2 uv0 = texcoords[vtxT[0] - 1];
			DirectX::XMFLOAT2 uv1 = texcoords[vtxT[1] - 1];
			DirectX::XMFLOAT2 uv2 = texcoords[vtxT[2] - 1];

			float s1 = uv1.x - uv0.x;
			float t1 = uv1.y - uv0.y;
			float s2 = uv2.x - uv0.x;
			float t2 = uv2.y - uv0.x;

			float inv = 1.0f / ((s1 * t2) - (s2 * t1));
			float Tx = inv * (t2 * Q1.x - t1 * Q2.x);
			float Ty = inv * (t2 * Q1.y - t1 * Q2.y);
			float Tz = inv * (t2 * Q1.z - t1 * Q2.z);

			DirectX::XMFLOAT3 T = { Tx, Ty, Tz };
			DirectX::XMVECTOR TV = XMLoadFloat3(&T);
			TV = DirectX::XMVector3Normalize(TV);

			DirectX::XMVECTOR N0V = XMLoadFloat3(&normals[vtxN[0] - 1]);
			DirectX::XMVECTOR N1V = XMLoadFloat3(&normals[vtxN[1] - 1]);
			DirectX::XMVECTOR N2V = XMLoadFloat3(&normals[vtxN[2] - 1]);

			DirectX::XMVECTOR B0V = DirectX::XMVector3Cross(N0V, TV);
			DirectX::XMVECTOR B1V = DirectX::XMVector3Cross(N0V, TV);
			DirectX::XMVECTOR B2V = DirectX::XMVector3Cross(N0V, TV);

			DirectX::XMVECTOR T0V = DirectX::XMVector3Cross(B0V, N0V);
			DirectX::XMVECTOR T1V = DirectX::XMVector3Cross(B1V, N1V);
			DirectX::XMVECTOR T2V = DirectX::XMVector3Cross(B2V, N2V);

			DirectX::XMFLOAT3 T0{};
			DirectX::XMFLOAT3 T1{};
			DirectX::XMFLOAT3 T2{};

			XMStoreFloat3(&T0, T0V);
			XMStoreFloat3(&T1, T1V);
			XMStoreFloat3(&T2, T2V);

			tangentsPerPositions[vtxV[0] - 1] = DirectX::XMVectorAdd(tangentsPerPositions[vtxV[0] - 1], T0V);
			tangentsPerPositions[vtxV[1] - 1] = DirectX::XMVectorAdd(tangentsPerPositions[vtxV[1] - 1], T1V);
			tangentsPerPositions[vtxV[2] - 1] = DirectX::XMVectorAdd(tangentsPerPositions[vtxV[2] - 1], T2V);

			for (unsigned i = 0; i != s.size() - 1; ++i)
			{
				std::vector<int> vertexData = { vtxV[i], vtxN[i], vtxT[i], vtxTan[i], vtxBit[i] };
				index[i] = int(vertices.size());
				vertices.push_back(vertexData);
			}

			indices.push_back({ index[0], index[2], index[1] });

			// I don't want to do 4th face so no.
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
					DirectX::XMFLOAT3 tangent{};
					XMStoreFloat3(&tangent, DirectX::XMVector3Normalize(tangentsPerPositions[it[0] - 1]));
					DirectX::XMVECTOR normal = XMLoadFloat3(&normals[it[1] - 1]);
					normal = DirectX::XMVector3Normalize(normal);
					XMStoreFloat3(&normals[it[1] - 1], normal);
					Vertex vtx{ positions[it[0] - 1], normals[it[1] - 1], texcoords[it[2] - 1], tangent };
					vertexBuffer[i] = vtx;

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
		DirectX::XMFLOAT3 tangent{};
		XMStoreFloat3(&tangent, DirectX::XMVector3Normalize(tangentsPerPositions[it[0] - 1]));
		DirectX::XMVECTOR normal = XMLoadFloat3(&normals[it[1] - 1]);
		normal = DirectX::XMVector3Normalize(normal);
		XMStoreFloat3(&normals[it[1] - 1], normal);
		Vertex vtx{ positions[it[0] - 1], normals[it[1] - 1], texcoords[it[2] - 1], tangent };
		vertexBuffer[i] = vtx;

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
		LOG_INFO << "MTL file \"" << mtlFile << "\" opened." << std::endl;
		std::string mtlLine;
		std::shared_ptr<BlinnPhongMaterial> current_mtl = nullptr;
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
				current_mtl = std::make_shared<BlinnPhongMaterial>(device);
			}
			else if (first_token == "Kd")
			{
				float r, g, b;
				str2num(s[1], r);
				str2num(s[2], g);
				str2num(s[3], b);
				current_mtl->parameters.diffuse = DirectX::XMFLOAT4(r, g, b, 1.0f);
			}
			else if (first_token == "Ka")
			{
				float r, g, b;
				str2num(s[1], r);
				str2num(s[2], g);
				str2num(s[3], b);
				current_mtl->parameters.ambient = DirectX::XMFLOAT4(r, g, b, 1.0f);
			}
			else if (first_token == "Ks")
			{
				float r, g, b;
				str2num(s[1], r);
				str2num(s[2], g);
				str2num(s[3], b);
				current_mtl->parameters.specular = DirectX::XMFLOAT4(r, g, b, 1.0f);
			}
			else if (first_token == "Ke")
			{
				float r, g, b;
				str2num(s[1], r);
				str2num(s[2], g);
				str2num(s[3], b);
				current_mtl->parameters.emission = DirectX::XMFLOAT4(r, g, b, 1.0f);
			}
			else if (first_token == "Ns")
			{
				//current_mtl->shininess = 20;
				str2num(s[1], current_mtl->parameters.shininess);
			}
			else if (first_token == "map_Kd")
			{
				// texture file
				std::string name = folder + s[1];
				// string -> wstring
				std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
				std::wstring wName = cv.from_bytes(name);
				HRESULT hr = DirectX::CreateWICTextureFromFileEx(device, context, wName.c_str(), D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
					0, 0, DirectX::WIC_LOADER_FORCE_SRGB, nullptr, &current_mtl->diffuseSrvPtr);
				if (FAILED(hr))
				{
					LOG_WARNING << "Failed to load diffuse texture file \"" << name << "\"." << std::endl;
				}
				else
				{
					LOG_INFO << "Load diffuse texture file \"" << name << "\"." << std::endl;
					current_mtl->InitializeSampler();
				}
			}
			else if (first_token == "map_Bump")
			{
				// texture file
				std::string name = folder + s[1];
				// string -> wstring
				std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
				std::wstring wName = cv.from_bytes(name);
				HRESULT hr = DirectX::CreateWICTextureFromFile(device, context, wName.c_str(), nullptr, &current_mtl->normalSrvPtr);
				if (FAILED(hr))
				{
					LOG_WARNING << "Failed to load normal texture file \"" << name << "\"." << std::endl;
				}
				else
				{
					LOG_INFO << "Load normal texture file \"" << name << "\"." << std::endl;
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
		LOG_INFO << "No mtl data in file \"" << filename << "\" found. Fallback to default material." << std::endl;
		materialList.push_back(BlinnPhongMaterial::GetDefault());
	}

	// Set Material of Mesh
	for (unsigned m = 0; m != meshList.size(); ++m)
	{
		meshList[m]->SetMaterial(materialMap[mtlOfMeshes[m]]);
	}


	std::pair<std::vector<std::shared_ptr<Mesh>>, std::vector<std::shared_ptr<BlinnPhongMaterial>>> result(meshList, materialList);
	return result;
}
