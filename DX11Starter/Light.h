#pragma once
#include <DirectXMath.h>

#define LIGHT_TYPE_DIR 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

struct Light
{
	int Type;
	DirectX::XMFLOAT3 Direction;// 16 bytes
	float Range;
	DirectX::XMFLOAT3 Position;// 32 bytes
	float Intensity;
	DirectX::XMFLOAT3 Color;// 48 bytes
	float SpotFalloff;
	DirectX::XMFLOAT3 Padding;// 64 bytes
};


Light DirectionalLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 direction, float intensity);
Light PointLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 position, float range, float intensity);
Light SpotLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 direction, float spotFalloff, float intensity);