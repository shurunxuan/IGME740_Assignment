#pragma once
#include <DirectXMath.h>

class FirstPersonCamera
{
public:
	FirstPersonCamera(float screenWidth, float screenHeight);
	~FirstPersonCamera();

	void Update(float deltaXt, float deltaYt, float deltaZt, float deltaXr, float deltaYr);
	void UpdateViewMatrix();
	void UpdateProjectionMatrix(float width, float height, float fov);

	DirectX::XMMATRIX& GetViewMatrix();
	DirectX::XMMATRIX& GetProjectionMatrix();

	DirectX::XMFLOAT3 GetForward() const;
	DirectX::XMFLOAT3 GetRight() const;

private:
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 direction;

	float xRotation;
	float yRotation;

	DirectX::XMMATRIX viewMatrix;
	DirectX::XMMATRIX projectionMatrix;
};

