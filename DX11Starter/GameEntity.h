#pragma once

#include <memory>
#include <DirectXMath.h>
#include "Mesh.h"
#include "Material.h"

class GameEntity
{
public:
	GameEntity();
	GameEntity(const std::shared_ptr<Mesh>& m, const std::shared_ptr<Material>& mat);
	~GameEntity();

	void SetTranslation(DirectX::XMFLOAT3 t);
	void SetScale(DirectX::XMFLOAT3 s);
	void SetRotation(DirectX::XMFLOAT4 r);

	DirectX::XMFLOAT3& GetTranslation();
	DirectX::XMFLOAT3& GetScale();
	DirectX::XMFLOAT4& GetRotation();

	DirectX::XMFLOAT4X4& GetWorldMatrix();

	Mesh* GetMesh() const;
	Material* GetMaterial() const;

	void MoveToward(DirectX::XMFLOAT3 direction, float distance);
	void RotateAxis(DirectX::XMFLOAT3 axis, float radian);

private:
	DirectX::XMFLOAT3 translation{};
	DirectX::XMFLOAT3 scale{};
	DirectX::XMFLOAT4 rotation{};

	DirectX::XMFLOAT4X4 worldMatrix{};

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;

	bool shouldUpdate = true;

	void UpdateWorldMatrix();
};
