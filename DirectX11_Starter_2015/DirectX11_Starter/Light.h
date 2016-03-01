#pragma once
#include <DirectXMath.h>
#include "Transform.h"

struct RenderLight {
	DirectX::XMFLOAT4 AmbientColor;
	DirectX::XMFLOAT4 DiffuseColor;
	DirectX::XMFLOAT3 Direction;
};

class GameLight
{
public:
	GameLight();
	GameLight(DirectX::XMFLOAT4 newAmbientColor, DirectX::XMFLOAT4 newDiffuseColor);
	~GameLight();

	Transform& GetTransform() { return transform; }
	//DO NOT pass get render light directly into a shaders set data
	RenderLight& GetRenderLight() { return renderLight; }
	RenderLight& GetRenderLightData();
private:
	RenderLight renderLight;
	Transform transform;
};