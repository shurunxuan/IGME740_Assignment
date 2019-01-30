#include "GameEntity.h"

GameEntity::GameEntity()
{
	meshCount = 0;
	meshes = nullptr;
	InitializeTransform();

	printf("[INFO] GameEntity created at <0x%p> by GameEntity::GameEntity().\n", this);

}

GameEntity::GameEntity(const std::shared_ptr<Mesh>& m)
{
	meshCount = 1;
	meshes = new std::shared_ptr<Mesh>[1];
	meshes[0] = m;
	InitializeTransform();

	printf("[INFO] GameEntity created at <0x%p> by GameEntity::GameEntity(const std::shared_ptr<Mesh> m).\n", this);
}

GameEntity::GameEntity(std::vector<std::shared_ptr<Mesh>> m)
{
	meshCount = int(m.size());
	meshes = new std::shared_ptr<Mesh>[meshCount];
	for (int i = 0; i != meshCount; ++i)
	{
		meshes[i] = m[i];
	}
	InitializeTransform();

	printf("[INFO] GameEntity created at <0x%p> by GameEntity::GameEntity(std::vector<std::shared_ptr<Mesh>> m).\n", this);
}

GameEntity::~GameEntity()
{
	delete[] meshes;
	printf("[INFO] GameEntity destroyed at <0x%p>.\n", this);
}

void GameEntity::UpdateWorldMatrix()
{
	const DirectX::XMMATRIX t = DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);
	const DirectX::XMMATRIX r = DirectX::XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));
	const DirectX::XMMATRIX s = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);

	const DirectX::XMMATRIX w = s * r * t;

	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(w));
	XMStoreFloat4x4(&itWorldMatrix, XMMatrixInverse(nullptr, w));

	shouldUpdate = false;
	//printf("[INFO] WorldMatrix of Game Entity <0x%p> Updated.\n", this);
}

void GameEntity::SetTranslation(const DirectX::XMFLOAT3 t)
{
	translation = t;
	shouldUpdate = true;
}

void GameEntity::SetScale(const DirectX::XMFLOAT3 s)
{
	scale = s;
	shouldUpdate = true;
}

void GameEntity::SetRotation(const DirectX::XMFLOAT4 r)
{
	const DirectX::XMVECTOR rVec = DirectX::XMQuaternionNormalize(DirectX::XMLoadFloat4(&r));
	DirectX::XMStoreFloat4(&rotation, rVec);
	shouldUpdate = true;
}

DirectX::XMFLOAT3& GameEntity::GetTranslation()
{
	return translation;
}

DirectX::XMFLOAT3& GameEntity::GetScale()
{
	return scale;
}

DirectX::XMFLOAT4& GameEntity::GetRotation()
{
	return rotation;
}

DirectX::XMFLOAT4X4& GameEntity::GetWorldMatrix()
{
	if (shouldUpdate)
	{
		UpdateWorldMatrix();
	}
	return worldMatrix;
}

DirectX::XMFLOAT4X4& GameEntity::GetWorldMatrixIT()
{
	if (shouldUpdate)
	{
		UpdateWorldMatrix();
	}
	return itWorldMatrix;
}

int GameEntity::GetMeshCount() const
{
	return meshCount;
}

Mesh* GameEntity::GetMeshAt(int index) const
{
	return meshes[index].get();
}

void GameEntity::MoveToward(DirectX::XMFLOAT3 direction, const float distance)
{
	const DirectX::XMVECTOR dir = XMLoadFloat3(&direction);
	DirectX::XMVECTOR t = XMLoadFloat3(&translation);

	t = DirectX::XMVectorAdd(t, DirectX::XMVectorScale(dir, distance));

	DirectX::XMStoreFloat3(&translation, t);
}

void GameEntity::RotateAxis(DirectX::XMFLOAT3 axis, float radian)
{
	const DirectX::XMVECTOR rot = DirectX::XMQuaternionRotationAxis(XMLoadFloat3(&axis), radian);
	DirectX::XMVECTOR cur = XMLoadFloat4(&rotation);

	cur = DirectX::XMQuaternionMultiply(cur, rot);

	XMStoreFloat4(&rotation, cur);
}

void GameEntity::InitializeTransform()
{
	translation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	XMStoreFloat4(&rotation, DirectX::XMQuaternionIdentity());
	XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixIdentity());
}


