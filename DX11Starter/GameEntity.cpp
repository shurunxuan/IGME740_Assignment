#include "GameEntity.h"

GameEntity::GameEntity()
{
	mesh = nullptr;
	translation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f); 
	scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	XMStoreFloat4(&rotation, DirectX::XMQuaternionIdentity());
	XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixIdentity());

	printf("[INFO] GameEntity created at <0x%p> by GameEntity::GameEntity().\n", this);

}

GameEntity::GameEntity(const std::shared_ptr<Mesh>& m)
{
	mesh = m;
	translation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	XMStoreFloat4(&rotation, DirectX::XMQuaternionIdentity());
	XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixIdentity());

	printf("[INFO] GameEntity created at <0x%p> by GameEntity::GameEntity(const std::shared_ptr<Mesh> m).\n", this);
}

GameEntity::~GameEntity() 
{
	printf("[INFO] GameEntity destroyed at <0x%p>.\n", this);
}

void GameEntity::UpdateWorldMatrix()
{
	const DirectX::XMMATRIX t = DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);
	const DirectX::XMMATRIX r = DirectX::XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));
	const DirectX::XMMATRIX s = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);

	const DirectX::XMMATRIX w = s * r * t;

	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(w));

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
	rotation = r;
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

Mesh* GameEntity::GetMesh() const
{
	return mesh.get();
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


