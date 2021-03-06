#pragma once

#include "DXCore.h"
#include "SimpleShader.h"
#include "GameEntity.h"
#include "FirstPersonCamera.h"
#include "Light.h"
#include <fstream>
#include "Skybox.h"
#include <DirectXCollision.h>

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

	// Overridden mouse input helper methods
	void OnMouseDown (WPARAM buttonState, int x, int y);
	void OnMouseUp	 (WPARAM buttonState, int x, int y);
	void OnMouseMove (WPARAM buttonState, int x, int y);
	void OnMouseWheel(float wheelDelta,   int x, int y);
private:

	// Buffers to hold actual geometry data
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;

	// Alpha Blending 
	ID3D11BlendState* blendState;
	// Depth Stencil Test
	ID3D11DepthStencilState* depthStencilState;

	// Front Face Culling & Back Face Culling render states
	ID3D11RasterizerState* drawingRenderState;
	ID3D11RasterizerState* shadowRenderState;
	ID3D11SamplerState* comparisonSampler;

	// Function for post processing
	void PostRender(int resourceIndex, int targetIndex, SimplePixelShader* pixelShader, const std::vector<std::pair<std::string, std::pair<void*, unsigned>>>& data = std::vector<std::pair<std::string, std::pair<void*, unsigned>>>());
	void PostRenderAdd(int resourceIndex0, int resourceIndex1, int targetIndex);
	void PostRenderCopy(int resourceIndex, int targetIndex, float xScale = 1.0f, float yScale = 1.0f);

	// Wrappers for DirectX shaders to provide simplified functionality
	SimpleVertexShader* vertexShader;
	SimplePixelShader* blinnPhongPixelShader;
	SimplePixelShader* brdfPixelShader;
	SimpleVertexShader* skyboxVertexShader;
	SimplePixelShader* skyboxPixelShader;
	SimpleVertexShader* postProcessingVertexShader;
	SimplePixelShader* ppAddShader;
	SimplePixelShader* ppCopyShader;
	SimplePixelShader* ppDarkCornerShader;
	SimplePixelShader* ppGaussianBlurUShader;
	SimplePixelShader* ppGaussianBlurVShader;

	// Keeps track of the old mouse position.  Useful for 
	// determining how far the mouse moved in a single frame.
	POINT prevMousePos;

	// Bounding box
	DirectX::XMVECTOR sceneAABBMin;
	DirectX::XMVECTOR sceneAABBMax;

	// Skybox
	int skyboxCount;
	int currentSkybox;
	Skybox** skyboxes;

	// Store the GameEntity data
	int entityCount;
	GameEntity** entities;

	// Camera
	FirstPersonCamera* camera;

	// Lighting
	int lightCount;
	Light** lights;
	LightStructure* lightData;
	SimpleVertexShader* shadowVertexShader;
	SimplePixelShader* shadowPixelShader;
};

