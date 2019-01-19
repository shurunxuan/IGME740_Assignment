#pragma once
#include <DirectXMath.h>

class FirstPersonCamera
{
public:
	FirstPersonCamera();
	~FirstPersonCamera();

private:
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 direction;

};

