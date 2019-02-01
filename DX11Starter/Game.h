#pragma once

#include "DXCore.h"
#include "SimpleShader.h"
#include "GameEntity.h"
#include "FirstPersonCamera.h"
#include "Light.h"
#include <fstream>
#include "Skybox.h"

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

	// Initialization helper methods - feel free to customize, combine, etc.
	void LoadShaders(); 
	void CreateMatrices();
	void CreateBasicGeometry();

	// Buffers to hold actual geometry data
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;

	// Alpha Blending 
	ID3D11BlendState* blendState;
	// Depth Stencil Test
	ID3D11DepthStencilState* depthStencilState;

	// Wrappers for DirectX shaders to provide simplified functionality
	SimpleVertexShader* vertexShader;
	SimplePixelShader* blinnPhongPixelShader;
	SimplePixelShader* brdfPixelShader;
	SimpleVertexShader* skyboxVertexShader;
	SimplePixelShader* skyboxPixelShader;

	// Keeps track of the old mouse position.  Useful for 
	// determining how far the mouse moved in a single frame.
	POINT prevMousePos;

	// Skybox
	int skyboxCount;
	int currentSkybox;
	Skybox** skyboxes;

	// Pre Integrated
	ID3D11SamplerState* anisotropicSampler;
	ID3D11ShaderResourceView* preIntegratedSrv;

	// Store the GameEntity data
	int entityCount;
	GameEntity** entities;

	// Camera
	FirstPersonCamera* camera;

	// Lighting
	int lightCount;
	Light* lights;
};

