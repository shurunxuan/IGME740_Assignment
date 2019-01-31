#include "Light.h"

Light DirectionalLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 direction, float intensity, DirectX::XMFLOAT3 ambientColor)
{
	Light result{};
	result.Color = color;
	result.AmbientColor = ambientColor;
	result.Direction = direction;
	result.Intensity = intensity;
	result.Type = LIGHT_TYPE_DIR;
	return result;
}

Light PointLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 position, float range, float intensity)
{
	Light result{};
	result.Color = color;
	result.Position = position;
	result.Range = range;
	result.Intensity = intensity;
	result.Type = LIGHT_TYPE_POINT;
	return result;
}

Light SpotLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 direction, float range, float spotFalloff,
	float intensity)
{
	Light result{};
	result.Color = color;
	result.Position = position;
	result.Direction = direction;
	result.Range = range;
	result.SpotFalloff = spotFalloff;
	result.Intensity = intensity;
	result.Type = LIGHT_TYPE_SPOT;
	return result;
}
