#pragma once

#include <memory>
#include <DirectXMath.h>
#include "Mesh.h"

class GameEntity
{
public:
	GameEntity();
	GameEntity(std::shared_ptr<Mesh> m);
	~GameEntity() = default;

	void SetTranslation(DirectX::XMFLOAT3 t);
	void SetScale(DirectX::XMFLOAT3 s);
	void SetRotation(DirectX::XMFLOAT4 r);

	DirectX::XMFLOAT3& GetTranslation();
	DirectX::XMFLOAT3& GetScale();
	DirectX::XMFLOAT4& GetRotation();

	DirectX::XMFLOAT4X4& GetWorldMatrix();

	void MoveToward(DirectX::XMFLOAT3 direction, float distance);
	void RotateAxis(DirectX::XMFLOAT3 axis, float radian);

private:
	DirectX::XMFLOAT3 translation{};
	DirectX::XMFLOAT3 scale{};
	DirectX::XMFLOAT4 rotation{};

	DirectX::XMFLOAT4X4 worldMatrix{};

	std::shared_ptr<Mesh> mesh;

	bool shouldUpdate = true;

	void UpdateWorldMatrix();
};
