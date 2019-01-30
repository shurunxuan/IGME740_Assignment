#include <cstdio>
#include "FirstPersonCamera.h"


FirstPersonCamera::FirstPersonCamera(float screenWidth, float screenHeight)
{
	xRotation = 0.0f;
	yRotation = 0.0f;

	position = DirectX::XMFLOAT3(0.0f, 0.0f, -5.0f);

	DirectX::XMFLOAT3 z(0.0f, 0.0f, 1.0f);
	const DirectX::XMVECTOR rotationQua = DirectX::XMQuaternionRotationRollPitchYaw(yRotation, xRotation, 0.0f);
	const DirectX::XMVECTOR newDirection = DirectX::XMVector3Rotate(XMLoadFloat3(&z), rotationQua);
	XMStoreFloat3(&direction, newDirection);
	UpdateViewMatrix();

	UpdateProjectionMatrix(screenWidth, screenHeight, 3.14159265f / 4.0f);

	printf("[INFO] FirstPersonCamera created at <0x%p> by FirstPersonCamera::FirstPersonCamera(float screenWidth, float screenHeight).\n", this);
}


FirstPersonCamera::~FirstPersonCamera()
{
	printf("[INFO] FirstPersonCamera destroyed at <0x%p>.\n", this);
}

void FirstPersonCamera::Update(float deltaXt, float deltaYt, float deltaZt, float deltaXr, float deltaYr)
{
	// Rotate direction
	xRotation += deltaXr;
	//if (xRotation > 3.1415927f * 2) xRotation -= 3.1415927f * 2;
	//else if (xRotation < 0) xRotation += 3.1415927f * 2;
	yRotation += deltaYr;
	if (yRotation > 80.0f / 180 * 3.1415927f) yRotation = 80.0f / 180 * 3.1415927f;
	else if (yRotation < -80.0f / 180 * 3.1415927f) yRotation = -80.0f / 180 * 3.1415927f;

	DirectX::XMFLOAT3 z(0.0f, 0.0f, 1.0f);
	const DirectX::XMVECTOR rotationQua = DirectX::XMQuaternionRotationRollPitchYaw(yRotation, xRotation, 0.0f);
	const DirectX::XMVECTOR newDirection = DirectX::XMVector3Rotate(XMLoadFloat3(&z), rotationQua);
	XMStoreFloat3(&direction, newDirection);

	// Translate position
	DirectX::XMFLOAT3 translate(deltaXt, deltaYt, deltaZt);
	const DirectX::XMVECTOR positionVec = XMLoadFloat3(&translate);
	const DirectX::XMVECTOR newPosition = DirectX::XMVectorAdd(XMLoadFloat3(&position), positionVec);
	XMStoreFloat3(&position, newPosition);
}

void FirstPersonCamera::UpdateViewMatrix()
{
	// 4D Vectors
	DirectX::XMFLOAT4 position4(position.x, position.y, position.z, 0.0f);
	DirectX::XMFLOAT4 direction4(direction.x, direction.y, direction.z, 0.0f);
	// Create the View matrix
	// - In an actual game, recreate this matrix every time the camera 
	//    moves (potentially every frame)
	// - We're using the LOOK TO function, which takes the position of the
	//    camera and the direction vector along which to look (as well as "up")
	// - Another option is the LOOK AT function, to look towards a specific
	//    point in 3D space
	const DirectX::XMVECTOR up = DirectX::XMVectorSet(0, 1, 0, 0);
	const DirectX::XMMATRIX V = DirectX::XMMatrixLookToLH(
		XMLoadFloat4(&position4),     // The position of the "camera"
		XMLoadFloat4(&direction4),     // Direction the camera is looking
		up);     // "Up" direction in 3D space (prevents roll)
	viewMatrix = XMMatrixTranspose(V); // Transpose for HLSL!
}

void FirstPersonCamera::UpdateProjectionMatrix(float width, float height, float fov)
{
	// Create the Projection matrix
	// - This should match the window's aspect ratio, and also update anytime
	//    the window resizes (which is already happening in OnResize() below)
	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(
		fov,		// Field of View Angle
		float(width) / height,		// Aspect ratio
		0.1f,						// Near clip plane distance
		1000.0f);					// Far clip plane distance
	projectionMatrix = XMMatrixTranspose(P); // Transpose for HLSL!
}

DirectX::XMMATRIX& FirstPersonCamera::GetViewMatrix()
{
	return viewMatrix;
}

DirectX::XMMATRIX& FirstPersonCamera::GetProjectionMatrix()
{
	return projectionMatrix;
}

DirectX::XMFLOAT3 FirstPersonCamera::GetForward() const
{
	const DirectX::XMVECTOR normForward = DirectX::XMVector3Normalize(XMLoadFloat3(&direction));
	DirectX::XMFLOAT3 result{};
	XMStoreFloat3(&result, normForward);
	return result;
}

DirectX::XMFLOAT3 FirstPersonCamera::GetRight() const
{
	DirectX::XMFLOAT3 up(0.0f, 1.0f, 0.0f);
	const DirectX::XMVECTOR upVec = XMLoadFloat3(&up);
	const DirectX::XMVECTOR forwardVec = XMLoadFloat3(&direction);

	const DirectX::XMVECTOR rightVec = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(upVec, forwardVec));

	DirectX::XMFLOAT3 right{};
	XMStoreFloat3(&right, rightVec);

	return right;
}
