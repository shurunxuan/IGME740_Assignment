#include <map>
#include <array>
#include "Game.h"
#include "Vertex.h"
#include "SimpleLogger.h"
#include <iostream>
#include <fstream>
#include <DDSTextureLoader.h>
#include "BrdfMaterial.h"
#include <codecvt>
#include <WICTextureLoader.h>
#include "BlinnPhongMaterial.h"

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,		// The application's handle
		"DirectX Game",	   	// Text for the window's title bar
		1280,			// Width of the window's client area
		720,			// Height of the window's client area
		true)			// Show extra stats (fps) in title bar?
{
	// Initialize fields
	vertexBuffer = 0;
	indexBuffer = 0;

	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	// Release any (and all!) DirectX objects
	// we've made in the Game class
	if (vertexBuffer) { vertexBuffer->Release(); }
	if (indexBuffer) { indexBuffer->Release(); }

	if (blendState) { blendState->Release(); }
	if (depthStencilState) { depthStencilState->Release(); }
	// Delete our simple shader objects, which
	// will clean up their own internal DirectX stuff
	delete vertexShader;
	delete blinnPhongPixelShader;
	delete brdfPixelShader;
	delete skyboxVertexShader;
	delete skyboxPixelShader;
	delete shadowVertexShader;
	delete shadowPixelShader;

	if (drawingRenderState) { drawingRenderState->Release(); }
	if (shadowRenderState) { shadowRenderState->Release(); }
	if (comparisonSampler) { comparisonSampler->Release(); }

	// Delete GameEntity data
	for (int i = 0; i < entityCount; ++i)
	{
		// TODO: Potential Access Violation not handled.
		delete entities[i];
	}
	delete[] entities;

	for (int i = 0; i < skyboxCount; ++i)
	{
		delete skyboxes[i];
	}
	delete[] skyboxes;

	// Delete Camera
	delete camera;

	// Delete Light
	delete[] lightData;

	for (int i = 0; i < lightCount; ++i)
	{
		delete lights[i];
	}
	delete[] lights;

}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Initialize Loggers
	ADD_LOGGER(info, std::cout);

	vertexShader = new SimpleVertexShader(device, context);
	vertexShader->LoadShaderFile(L"VertexShader.cso");

	blinnPhongPixelShader = new SimplePixelShader(device, context);
	blinnPhongPixelShader->LoadShaderFile(L"BlinnPhong.cso");

	brdfPixelShader = new SimplePixelShader(device, context);
	brdfPixelShader->LoadShaderFile(L"BRDF.cso");

	skyboxVertexShader = new SimpleVertexShader(device, context);
	skyboxVertexShader->LoadShaderFile(L"SkyboxVS.cso");

	skyboxPixelShader = new SimplePixelShader(device, context);
	skyboxPixelShader->LoadShaderFile(L"SkyboxPS.cso");

	shadowVertexShader = new SimpleVertexShader(device, context);
	shadowVertexShader->LoadShaderFile(L"ShadowVS.cso");

	shadowPixelShader = new SimplePixelShader(device, context);
	shadowPixelShader->LoadShaderFile(L"ShadowPS.cso");

	BlinnPhongMaterial::GetDefault()->SetVertexShaderPtr(vertexShader);
	BlinnPhongMaterial::GetDefault()->SetPixelShaderPtr(blinnPhongPixelShader);

	entityCount = 2;
	entities = new GameEntity * [entityCount];

	// Create GameEntity & Initial Transform
	const auto modelData1 = Mesh::LoadFromFile("models\\Rock\\sphere.obj", device, context);
	const auto modelData2 = Mesh::LoadFromFile("models\\Rock\\quad.obj", device, context);

	//for (int i = 0; i < 10; ++i)
	//for (int j = 0; j < 10; ++j)
	//{
	//	entities[10 * i + j] = new GameEntity(modelData1.first);
	//	entities[10 * i + j]->SetScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
	//	entities[10 * i + j]->SetTranslation(XMFLOAT3(3.0f * (j - 9.0f / 2.0f), 1.0f, 3.0f * (i - 9.0f / 2.0f)));
	//}
	entities[0] = new GameEntity(modelData1.first);
	entities[0]->SetScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
	entities[0]->SetTranslation(XMFLOAT3(0.0f, 1.0f, 0.0f));

	entities[entityCount - 1] = new GameEntity(modelData2.first);
	entities[entityCount - 1]->SetScale(XMFLOAT3(1.0f, 100.0f, 100.0f));
	XMFLOAT3 zAxis{ 0.0f, 0.0f, 1.0f };
	XMFLOAT4 q{};
	XMVECTOR z = XMLoadFloat3(&zAxis);
	XMVECTOR rq = XMQuaternionRotationAxis(z, XM_PIDIV2);
	XMStoreFloat4(&q, rq);
	entities[entityCount - 1]->SetRotation(q);

	//entities[2] = new GameEntity(modelData1.first);
	//entities[2]->SetScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
	//entities[2]->SetTranslation(XMFLOAT3(0.0f, -2.0f, 0.0f));

	for (int k = 0; k < entities[0]->GetMeshCount(); ++k)
	{
		auto originalMaterial = entities[0]->GetMeshAt(k)->GetMaterial();
		// Test the new BRDF Material
		std::shared_ptr<BrdfMaterial> brdfMaterial = std::make_shared<BrdfMaterial>(vertexShader, brdfPixelShader, device);
		// Gold
		brdfMaterial->parameters.albedo = { 1.000000f, 0.765557f, 0.336057f };
		brdfMaterial->parameters.roughness = 0.5f;
		brdfMaterial->parameters.metalness = 0.1f;
		ID3D11Resource* resource;
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		if (originalMaterial->diffuseSrvPtr)
		{
			originalMaterial->diffuseSrvPtr->GetResource(&resource);
			originalMaterial->diffuseSrvPtr->GetDesc(&srvDesc);
			device->CreateShaderResourceView(resource, &srvDesc, &brdfMaterial->diffuseSrvPtr);
			resource->Release();
		}
		if (originalMaterial->normalSrvPtr)
		{
			originalMaterial->normalSrvPtr->GetResource(&resource);
			originalMaterial->normalSrvPtr->GetDesc(&srvDesc);
			device->CreateShaderResourceView(resource, &srvDesc, &brdfMaterial->normalSrvPtr);
			resource->Release();
		}
		brdfMaterial->InitializeSampler();
		entities[0]->GetMeshAt(k)->SetMaterial(brdfMaterial);
	}

	for (int k = 0; k < entities[entityCount - 1]->GetMeshCount(); ++k)
	{
		auto originalMaterial = entities[entityCount - 1]->GetMeshAt(k)->GetMaterial();
		// Test the new BRDF Material
		std::shared_ptr<BrdfMaterial> brdfMaterial = std::make_shared<BrdfMaterial>(vertexShader, brdfPixelShader, device);
		// Gold
		brdfMaterial->parameters.albedo = { 0.5f, 0.5f, 0.5f };
		brdfMaterial->parameters.roughness = 1.0f;
		brdfMaterial->parameters.metalness = 0.0f;
		ID3D11Resource* resource;
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		if (originalMaterial->diffuseSrvPtr)
		{
			originalMaterial->diffuseSrvPtr->GetResource(&resource);
			originalMaterial->diffuseSrvPtr->GetDesc(&srvDesc);
			device->CreateShaderResourceView(resource, &srvDesc, &brdfMaterial->diffuseSrvPtr);
			resource->Release();
		}
		if (originalMaterial->normalSrvPtr)
		{
			originalMaterial->normalSrvPtr->GetResource(&resource);
			originalMaterial->normalSrvPtr->GetDesc(&srvDesc);
			device->CreateShaderResourceView(resource, &srvDesc, &brdfMaterial->normalSrvPtr);
			resource->Release();
		}
		brdfMaterial->InitializeSampler();
		entities[entityCount - 1]->GetMeshAt(k)->SetMaterial(brdfMaterial);
	}

	// Get the AABB bounding box of the scene
	XMVECTOR vMeshMin;
	XMVECTOR vMeshMax;
	sceneAABBMin = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
	sceneAABBMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
	// Calculate the AABB for the scene by iterating through all the meshes in the SDKMesh file.
	for (int i = 0; i < entityCount; ++i)
	{
		for (int j = 0; j < entities[i]->GetMeshCount(); ++j)
		{
			auto msh = entities[i]->GetMeshAt(j);
			XMFLOAT3 bbCenter = msh->BoundingBoxCenter;
			XMFLOAT3 bbExtent = msh->BoundingBoxExtents;
			XMVECTOR cen = XMLoadFloat3(&bbCenter);
			XMVECTOR ext = XMLoadFloat3(&bbExtent);
			XMVECTOR min = cen - ext;
			XMVECTOR max = cen + ext;


			XMFLOAT4X4 worldMat = entities[i]->GetWorldMatrix();
			XMMATRIX mat = XMLoadFloat4x4(&worldMat);
			mat = XMMatrixTranspose(mat);

			BoundingBox bb;
			BoundingBox::CreateFromPoints(bb, min, max);
			bb.Transform(bb, mat);

			vMeshMin = XMVectorSet(bb.Center.x - bb.Extents.x,
				bb.Center.y - bb.Extents.y,
				bb.Center.z - bb.Extents.z,
				1.0f);

			vMeshMax = XMVectorSet(bb.Center.x + bb.Extents.x,
				bb.Center.y + bb.Extents.y,
				bb.Center.z + bb.Extents.z,
				1.0f);

			sceneAABBMin = XMVectorMin(vMeshMin, sceneAABBMin);
			sceneAABBMax = XMVectorMax(vMeshMax, sceneAABBMax);
		}
	}

	XMFLOAT4 aabbMin{};
	XMFLOAT4 aabbMax{};
	DirectX::XMStoreFloat4(&aabbMin, sceneAABBMin);
	DirectX::XMStoreFloat4(&aabbMax, sceneAABBMax);

	LOG_DEBUG << "AABB MIN: x = " << aabbMin.x << ", y = " << aabbMin.y << ", z = " << aabbMin.z << ", w = " << aabbMin.w << std::endl;
	LOG_DEBUG << "AABB MAX: x = " << aabbMax.x << ", y = " << aabbMax.y << ", z = " << aabbMax.z << ", w = " << aabbMax.w << std::endl;

	skyboxCount = 3;
	skyboxes = new Skybox * [skyboxCount];
	skyboxes[0] = new Skybox(device, context, "models\\Skyboxes\\Environment2HiDef.cubemap.dds", "models\\Skyboxes\\Environment2Light.cubemap.dds");
	skyboxes[1] = new Skybox(device, context, "models\\Skyboxes\\Environment3HiDef.cubemap.dds", "models\\Skyboxes\\Environment3Light.cubemap.dds");
	skyboxes[2] = new Skybox(device, context, "models\\Skyboxes\\Environment1HiDef.cubemap.dds", "models\\Skyboxes\\Environment1Light.cubemap.dds");
	currentSkybox = 0;
	for (int i = 0; i < skyboxCount; ++i)
	{
		skyboxes[i]->SetVertexShader(skyboxVertexShader);
		skyboxes[i]->SetPixelShader(skyboxPixelShader);
	}

	// Initialize Light
	lightCount = 3;
	lightData = new LightStructure[lightCount];

	//lightData[0] = DirectionalLight(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 1.0f, 0.0f), 1.0f, XMFLOAT3(0.0f, 0.0f, 0.0f));
	//lightData[1] = PointLight(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(2.0f, 0.0f, 0.0f), 3.0f, 1.0f);
	//lightData[2] = SpotLight(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -2.0f), XMFLOAT3(0.0f, 0.5f, 1.0f), 10.0f, 0.8f, 1.0f);
	lightData[0] = DirectionalLight(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 0.0f), 1.0f, XMFLOAT3(1.0f, 1.0f, 1.0f));
	//lightData[1] = PointLight(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(2.0f, 3.0f, 0.0f), 8.0f, 1.0f);
	//lightData[2] = SpotLight(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 3.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), 10.0f, 0.8f, 1.0f);

	// Create FirstPersonCamera
	camera = new FirstPersonCamera(float(width), float(height));

	lights = new Light * [lightCount];

	for (int i = 0; i < lightCount; ++i)
	{
		lights[i] = new Light(lightData + i, device, context, camera, sceneAABBMin, sceneAABBMax);
	}

	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Alpha Blending
	D3D11_BLEND_DESC BlendState;
	ZeroMemory(&BlendState, sizeof(D3D11_BLEND_DESC));
	BlendState.RenderTarget[0].BlendEnable = TRUE;
	BlendState.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendState.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	BlendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendState.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	device->CreateBlendState(&BlendState, &blendState);
	context->OMSetBlendState(blendState, nullptr, 0xFFFFFF);

	// Depth Stencil Test
	D3D11_DEPTH_STENCIL_DESC dsDesc;

	// Depth test parameters
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	// Stencil test parameters
	dsDesc.StencilEnable = false;
	dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	// Stencil operations if pixel is front-facing
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create depth stencil state
	device->CreateDepthStencilState(&dsDesc, &depthStencilState);
	context->OMSetDepthStencilState(depthStencilState, 0);

	D3D11_SAMPLER_DESC comparisonSamplerDesc;
	ZeroMemory(&comparisonSamplerDesc, sizeof(D3D11_SAMPLER_DESC));
	comparisonSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.BorderColor[0] = 1.0f;
	comparisonSamplerDesc.BorderColor[1] = 1.0f;
	comparisonSamplerDesc.BorderColor[2] = 1.0f;
	comparisonSamplerDesc.BorderColor[3] = 1.0f;
	comparisonSamplerDesc.MinLOD = 0.f;
	comparisonSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	comparisonSamplerDesc.MipLODBias = 0.f;
	comparisonSamplerDesc.MaxAnisotropy = 0;
	comparisonSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	comparisonSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	//comparisonSamplerDesc.MaxAnisotropy = 16;

	// Point filtered shadows can be faster, and may be a good choice when
	// rendering on hardware with lower feature levels. This sample has a
	// UI option to enable/disable filtering so you can see the difference
	// in quality and speed.

	device->CreateSamplerState(
		&comparisonSamplerDesc,
		&comparisonSampler
	);

	D3D11_RASTERIZER_DESC drawingRenderStateDesc;
	ZeroMemory(&drawingRenderStateDesc, sizeof(D3D11_RASTERIZER_DESC));
	drawingRenderStateDesc.CullMode = D3D11_CULL_BACK;
	drawingRenderStateDesc.FillMode = D3D11_FILL_SOLID;
	drawingRenderStateDesc.DepthClipEnable = true; // Feature level 9_1 requires DepthClipEnable == true
	device->CreateRasterizerState(
		&drawingRenderStateDesc,
		&drawingRenderState
	);

	D3D11_RASTERIZER_DESC shadowRenderStateDesc;
	ZeroMemory(&shadowRenderStateDesc, sizeof(D3D11_RASTERIZER_DESC));
	shadowRenderStateDesc.CullMode = D3D11_CULL_FRONT;
	shadowRenderStateDesc.FillMode = D3D11_FILL_SOLID;
	shadowRenderStateDesc.DepthBias = 100;
	shadowRenderStateDesc.DepthBiasClamp = 0.1f;
	shadowRenderStateDesc.SlopeScaledDepthBias = 1.0f;
	shadowRenderStateDesc.DepthClipEnable = true;

	device->CreateRasterizerState(
		&shadowRenderStateDesc,
		&shadowRenderState
	);
}


// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	camera->UpdateProjectionMatrix(float(width), float(height), 3.14159265f / 4.0f);
}

// Several small variables to record the direction of the animation
bool animateLight = false;
bool animateModel = false;
bool turnOnNormalMap = true;
bool visualizeCascade = false;
bool rotateSkybox = false;
bool modelAnimationDir = false;

float cascadeBlendArea = 0.001f;

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	if (animateLight)
	{
		XMFLOAT3 yAxis = { 0.0f, 1.0f, 0.0f };
		XMVECTOR yVec = XMLoadFloat3(&yAxis);
		XMVECTOR rotateQ = XMQuaternionRotationAxis(yVec, deltaTime / 3.0f);
		XMVECTOR lightDirection = XMLoadFloat3(&lightData[0].Direction);
		lightDirection = XMVector3Rotate(lightDirection, rotateQ);
		XMFLOAT3 newLightDirection{};
		XMStoreFloat3(&newLightDirection, lightDirection);
		lights[0]->SetDirection(newLightDirection);

		XMVECTOR lightPosition = XMLoadFloat3(&lightData[1].Position);
		lightPosition = XMVector3Rotate(lightPosition, rotateQ);
		XMStoreFloat3(&lightData[1].Position, lightPosition);

		XMFLOAT3 xAxis = { 1.0f, 0.0f, 0.0f };
		XMVECTOR xVec = XMLoadFloat3(&xAxis);
		XMVECTOR rotateXQ = XMQuaternionRotationAxis(xVec, deltaTime);
		XMVECTOR spotLightDirection = XMLoadFloat3(&lightData[2].Direction);
		spotLightDirection = XMVector3Rotate(spotLightDirection, rotateXQ);
		XMStoreFloat3(&lightData[2].Direction, spotLightDirection);
	}

	if (animateModel)
	{
		//XMFLOAT3 yAxis = { 0.0f, 1.0f, 0.0f };
		//XMVECTOR yVec = XMLoadFloat3(&yAxis);
		//XMVECTOR rotateQ = XMQuaternionRotationAxis(yVec, deltaTime);
		//XMVECTOR modelRotation = XMLoadFloat4(&entities[0]->GetRotation());
		//modelRotation = XMQuaternionMultiply(modelRotation, rotateQ);
		//XMFLOAT4 rot{};
		//XMStoreFloat4(&rot, modelRotation);
		//entities[0]->SetRotation(rot);


		XMFLOAT3 translation = entities[0]->GetTranslation();
		if (translation.y > 1.0f) modelAnimationDir = false;
		else if (translation.y < -1.0f) modelAnimationDir = true;
		XMVECTOR t = XMLoadFloat3(&translation);
		XMFLOAT3 delta = { 0.0f, (modelAnimationDir ? 1.0f : -1.0f) * deltaTime, 0.0f };
		XMVECTOR d = XMLoadFloat3(&delta);

		XMStoreFloat3(&translation, XMVectorAdd(t, d));
		entities[0]->SetTranslation(translation);
	}

	if (rotateSkybox)
	{
		XMVECTOR r = skyboxes[currentSkybox]->GetRotationQuaternion();
		XMFLOAT3 yAxis = { 0.0f, 1.0f, 0.0f };
		XMVECTOR yVec = XMLoadFloat3(&yAxis);
		XMVECTOR rotateY = XMQuaternionRotationAxis(yVec, deltaTime);
		r = XMQuaternionMultiply(r, rotateY);
		skyboxes[currentSkybox]->SetRotationQuaternion(r);
	}

	// W, A, S, D for moving camera
	const XMFLOAT3 forward = camera->GetForward();
	const XMFLOAT3 right = camera->GetRight();
	const XMFLOAT3 up(0.0f, 1.0f, 0.0f);
	float speed = 2.0f * deltaTime;
	if (GetAsyncKeyState(VK_LSHIFT))
	{
		speed *= 2.0f;
	}
	if (GetAsyncKeyState('W') & 0x8000)
	{
		camera->Update(forward.x * speed, forward.y * speed, forward.z * speed, 0.0f, 0.0f);
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		camera->Update(-forward.x * speed, -forward.y * speed, -forward.z * speed, 0.0f, 0.0f);
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		camera->Update(right.x * speed, right.y * speed, right.z * speed, 0.0f, 0.0f);
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		camera->Update(-right.x * speed, -right.y * speed, -right.z * speed, 0.0f, 0.0f);
	}
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		camera->Update(up.x * speed, up.y * speed, up.z * speed, 0.0f, 0.0f);
	}
	if (GetAsyncKeyState('X') & 0x8000)
	{
		camera->Update(-up.x * speed, -up.y * speed, -up.z * speed, 0.0f, 0.0f);
	}

	// Animation
	if (GetAsyncKeyState('L') & 0x1)
	{
		animateLight = !animateLight;
	}
	if (GetAsyncKeyState('M') & 0x1)
	{
		animateModel = !animateModel;
	}
	if (GetAsyncKeyState('N') & 0x1)
	{
		turnOnNormalMap = !turnOnNormalMap;
	}
	if (GetAsyncKeyState('R') & 0x1)
	{
		rotateSkybox = !rotateSkybox;
	}
	if (GetAsyncKeyState('V') & 0x1)
	{
		visualizeCascade = !visualizeCascade;
	}

	// Change Skybox
	if (GetAsyncKeyState('K') & 0x1)
	{
		currentSkybox += 1;
		currentSkybox %= skyboxCount;
	}
	const float materialSpeed = 0.5f;

	// Set cascaded shadow map blend area
	if (GetAsyncKeyState('O') & 0x8000)
	{
		cascadeBlendArea -= 0.001f;
		if (cascadeBlendArea < 0.0f) cascadeBlendArea = 0.0f;
		LOG_DEBUG << cascadeBlendArea << std::endl;
	}
	if (GetAsyncKeyState('P') & 0x8000)
	{
		cascadeBlendArea += 0.001f;
		if (cascadeBlendArea > 1.0f) cascadeBlendArea = 1.0f;
		LOG_DEBUG << cascadeBlendArea << std::endl;
	}

	// Set Roughness
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		for (int i = 0; i < entityCount; ++i)
		{
			for (int j = 0; j < entities[i]->GetMeshCount(); ++j)
			{
				BrdfMaterial* material = reinterpret_cast<BrdfMaterial*>(entities[i]->GetMeshAt(j)->GetMaterial());
				material->parameters.roughness += materialSpeed * deltaTime;
				if (material->parameters.roughness > 1.0f) material->parameters.roughness = 1.0f;
			}
			break;
		}
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		for (int i = 0; i < entityCount; ++i)
		{
			for (int j = 0; j < entities[i]->GetMeshCount(); ++j)
			{
				BrdfMaterial* material = reinterpret_cast<BrdfMaterial*>(entities[i]->GetMeshAt(j)->GetMaterial());
				material->parameters.roughness -= materialSpeed * deltaTime;
				if (material->parameters.roughness < 0.0f) material->parameters.roughness = 0.0f;
			}
			break;
		}
	}
	// Set Metalness
	if (GetAsyncKeyState(VK_UP) & 0x8000)
	{
		for (int i = 0; i < entityCount; ++i)
		{
			for (int j = 0; j < entities[i]->GetMeshCount(); ++j)
			{
				BrdfMaterial* material = reinterpret_cast<BrdfMaterial*>(entities[i]->GetMeshAt(j)->GetMaterial());
				material->parameters.metalness += materialSpeed * deltaTime;
				if (material->parameters.metalness > 1.0f) material->parameters.metalness = 1.0f;
			}
			break;
		}
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
	{
		for (int i = 0; i < entityCount; ++i)
		{
			for (int j = 0; j < entities[i]->GetMeshCount(); ++j)
			{
				BrdfMaterial* material = reinterpret_cast<BrdfMaterial*>(entities[i]->GetMeshAt(j)->GetMaterial());
				material->parameters.metalness -= materialSpeed * deltaTime;
				if (material->parameters.metalness < 0.0f) material->parameters.metalness = 0.0f;
			}
			break;
		}
	}
	// Quit if the escape key is pressed
	if (GetAsyncKeyState(VK_ESCAPE))
		Quit();
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------

void Game::Draw(float deltaTime, float totalTime)
{
	const float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//const float color[4] = { 0.4f, 0.6f, 0.75f, 0.0f };

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV, color);
	context->ClearDepthStencilView(
		depthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	for (int i = 0; i < lightCount; ++i)
	{
		context->ClearDepthStencilView(lights[i]->GetShadowDepthView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	// Update camera view matrix before drawing
	camera->UpdateViewMatrix();

	// Update light matrices
	for (int l = 0; l < lightCount; ++l)
	{
		lights[l]->UpdateMatrices();
	}

#pragma region PreProcessing
	// Render lights
	for (int l = 0; l < lightCount; ++l)
	{
		// Render all the objects in the scene that can cast shadows onto themselves or onto other objects.

		// Only bind the ID3D11DepthStencilView for output.
		context->OMSetRenderTargets(
			0,
			nullptr,
			lights[l]->GetShadowDepthView()
		);

		// Note that starting with the second frame, the previous call will display
		// warnings in VS debug output about forcing an unbind of the pixel shader
		// resource. This warning can be safely ignored when using shadow buffers
		// as demonstrated in this sample.

		// Set rendering state.
		context->RSSetState(shadowRenderState);

		// Render Scene from Light Cascade PoV
		for (int c = 0; c < lights[l]->GetCascadeCount(); ++c)
		{
			context->RSSetViewports(1, lights[l]->GetShadowViewportAt(c));
			for (int i = 0; i < entityCount; ++i)
			{
				for (int j = 0; j < entities[i]->GetMeshCount(); ++j)
				{
					XMFLOAT4X4 viewMat{};
					XMFLOAT4X4 projMat{};
					XMStoreFloat4x4(&viewMat, lights[l]->GetViewMatrix());
					XMStoreFloat4x4(&projMat, lights[l]->GetProjectionMatrixAt(c));
					bool result;
					result = shadowVertexShader->SetMatrix4x4("world", entities[i]->GetWorldMatrix());
					if (!result) LOG_WARNING << "Error setting parameter " << "world" << " to vertex shader. Variable not found." << std::endl;

					result = shadowVertexShader->SetMatrix4x4("view", viewMat);
					if (!result) LOG_WARNING << "Error setting parameter " << "view" << " to vertex shader. Variable not found." << std::endl;

					result = shadowVertexShader->SetMatrix4x4("projection", projMat);
					if (!result) LOG_WARNING << "Error setting parameter " << "projection" << " to vertex shader. Variable not found." << std::endl;

					shadowVertexShader->CopyAllBufferData();

					shadowVertexShader->SetShader();
					context->PSSetShader(nullptr, nullptr, 0);

					ID3D11Buffer * vertexBuffer = entities[i]->GetMeshAt(j)->GetVertexBuffer();
					ID3D11Buffer * indexBuffer = entities[i]->GetMeshAt(j)->GetIndexBuffer();
					// Set buffers in the input assembler
					//  - Do this ONCE PER OBJECT you're drawing, since each object might
					//    have different geometry.
					UINT stride = sizeof(Vertex);
					UINT offset = 0;
					context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
					context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

					context->DrawIndexed(
						entities[i]->GetMeshAt(j)->GetIndexCount(),		// The number of indices to use (we could draw a subset if we wanted)
						0,								// Offset to the first index we want to use
						0);								// Offset to add to each index when looking up vertices
				}
			}
		}

	}
#pragma endregion 
#pragma region Rendering
	// Render to screen
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = float(width);
	viewport.Height = float(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	context->RSSetViewports(1, &viewport);
	context->RSSetState(drawingRenderState);
	context->OMSetRenderTargets(1, &backBufferRTV, depthStencilView);

	for (int i = 0; i < entityCount; ++i)
	{
		for (int j = 0; j < entities[i]->GetMeshCount(); ++j)
		{
			// Send data to shader variables
			//  - Do this ONCE PER OBJECT you're drawing
			//  - This is actually a complex process of copying data to a local buffer
			//    and then copying that entire buffer to the GPU.  
			//  - The "SimpleShader" class handles all of that for you.
			XMFLOAT4X4 viewMat{};
			XMFLOAT4X4 projMat{};
			XMStoreFloat4x4(&viewMat, camera->GetViewMatrix());
			XMStoreFloat4x4(&projMat, camera->GetProjectionMatrix());
			//XMStoreFloat4x4(&viewMat, lights[0]->GetViewMatrix());
			//XMStoreFloat4x4(&projMat, lights[0]->GetProjectionMatrixAt(0));
			bool result;
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("world", entities[i]->GetWorldMatrix());
			if (!result) LOG_WARNING << "Error setting parameter " << "world" << " to vertex shader. Variable not found." << std::endl;

			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("itworld", entities[i]->GetWorldMatrixIT());
			if (!result) LOG_WARNING << "Error setting parameter " << "itworld" << " to vertex shader. Variable not found." << std::endl;

			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("view", viewMat);
			if (!result) LOG_WARNING << "Error setting parameter " << "view" << " to vertex shader. Variable not found." << std::endl;

			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("projection", projMat);
			if (!result) LOG_WARNING << "Error setting parameter " << "projection" << " to vertex shader. Variable not found." << std::endl;

			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetInt("lightCount", lightCount);
			if (!result) LOG_WARNING << "Error setting parameter " << "lightCount" << " to vertex shader. Variable not found." << std::endl;

			// Set lights' matrices
			if (lightCount > 0)
			{
				XMFLOAT4X4 lViewMat{};
				XMStoreFloat4x4(&lViewMat, lights[0]->GetViewMatrix());
				result = entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("lView", lViewMat);
				if (!result) LOG_WARNING << "Error setting parameter " << "lView" << " to vertex shader. Variable." << std::endl;

				result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetInt("m_iPCFBlurForLoopStart", 3 / -2);
				result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetInt("m_iPCFBlurForLoopEnd", 3 / 2 + 1);
				result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetFloat("m_fCascadeBlendArea", cascadeBlendArea);
				result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetFloat("m_fTexelSize", 1.0f / 2048.0f);
				result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetFloat("m_fNativeTexelSizeInX", 1.0f / 2048.0f / lights[0]->GetCascadeCount());
				result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetFloat("m_fShadowBiasFromGUI", 0);
				result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetFloat("m_fShadowPartitionSize", 1.0f / lights[0]->GetCascadeCount());

				XMMATRIX matTextureScale = XMMatrixScaling(0.5f, -0.5f, 1.0f);
				XMMATRIX matTextureTranslation = XMMatrixTranslation(.5f, .5f, 0.f);

				XMFLOAT4 m_vCascadeScale[3];
				XMFLOAT4 m_vCascadeOffset[3];
				for (int index = 0; index < lights[0]->GetCascadeCount(); ++index)
				{
					XMMATRIX mShadowTexture = XMMatrixTranspose(lights[0]->GetProjectionMatrixAt(index)) * matTextureScale * matTextureTranslation;
					m_vCascadeScale[index].x = XMVectorGetX(mShadowTexture.r[0]);
					m_vCascadeScale[index].y = XMVectorGetY(mShadowTexture.r[1]);
					m_vCascadeScale[index].z = XMVectorGetZ(mShadowTexture.r[2]);
					m_vCascadeScale[index].w = 1;

					XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&m_vCascadeOffset[index]), mShadowTexture.r[3]);
					m_vCascadeOffset[index].w = 0;
				}

				result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetData("m_vCascadeOffset", &m_vCascadeOffset, sizeof(XMFLOAT4) * 3);
				result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetData("m_vCascadeScale", &m_vCascadeScale, sizeof(XMFLOAT4) * 3);

				// The border padding values keep the pixel shader from reading the borders during PCF filtering.
				result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetFloat("m_fMaxBorderPadding", (2048.0f - 1.0f) / 2048.0f);
				result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetFloat("m_fMinBorderPadding", (1.0f) / 2048.0f);
				result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetInt("m_nCascadeLevels", lights[0]->GetCascadeCount());
				result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetInt("m_iVisualizeCascades", visualizeCascade ? 1 : 0);

				//result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetMatrix4x4("cascadeProjection", lProjMat);
				//if (!result) LOG_WARNING << "Error setting parameter " << "cascadeProjection" << " to pixel shader. Variable not found." << std::endl;
			}




			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetInt("lightCount", lightCount);
			if (!result) LOG_WARNING << "Error setting parameter " << "lightCount" << " to pixel shader. Variable not found." << std::endl;

			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetData(
				"lights",					// The name of the (eventual) variable in the shader
				lightData,							// The address of the data to copy
				sizeof(LightStructure) * 24);		// The size of the data to copy
			if (!result) LOG_WARNING << "Error setting parameter " << "lights" << " to pixel shader. Variable not found or size incorrect." << std::endl;


			void* materialData;
			const size_t materialSize = entities[i]->GetMeshAt(j)->GetMaterial()->GetMaterialStruct(&materialData);
			// Material Data
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetData(
				"material",
				materialData,
				int(materialSize)
			);
			if (!result) LOG_WARNING << "Error setting parameter " << "material" << " to pixel shader. Variable not found or size incorrect." << std::endl;


			const bool hasNormalMap = entities[i]->GetMeshAt(j)->GetMaterial()->normalSrvPtr != nullptr;
			const bool hasDiffuseTexture = entities[i]->GetMeshAt(j)->GetMaterial()->diffuseSrvPtr != nullptr;

			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetFloat("hasNormalMap", turnOnNormalMap && hasNormalMap ? 1.0f : 0.0f);
			if (!result) LOG_WARNING << "Error setting parameter " << "hasNormalMap" << " to pixel shader. Variable not found or size incorrect." << std::endl;
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetFloat("hasDiffuseTexture", hasDiffuseTexture ? 1.0f : 0.0f);
			if (!result) LOG_WARNING << "Error setting parameter " << "hasDiffuseTexture" << " to pixel shader. Variable not found or size incorrect." << std::endl;

			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetFloat3("CameraPosition", camera->GetPosition());
			if (!result) LOG_WARNING << "Error setting parameter " << "CameraPosition" << " to pixel shader. Variable not found or size incorrect." << std::endl;

			XMMATRIX skyboxRotationMatrix = XMMatrixTranspose(XMMatrixRotationQuaternion(XMQuaternionInverse(skyboxes[currentSkybox]->GetRotationQuaternion())));
			XMFLOAT4X4 m{};
			XMStoreFloat4x4(&m, skyboxRotationMatrix);
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetMatrix4x4("SkyboxRotation", m);
			if (!result) LOG_WARNING << "Error setting parameter " << "SkyboxRotation" << " to pixel shader. Variable not found or size incorrect." << std::endl;
			// Sampler and Texture
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetSamplerState("basicSampler", entities[i]->GetMeshAt(j)->GetMaterial()->GetSamplerState());
			if (!result) LOG_WARNING << "Error setting sampler state " << "basicSampler" << " to pixel shader. Variable not found." << std::endl;
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetSamplerState("shadowSampler", comparisonSampler);
			if (!result) LOG_WARNING << "Error setting sampler state " << "shadowSampler" << " to pixel shader. Variable not found." << std::endl;
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetShaderResourceView("diffuseTexture", entities[i]->GetMeshAt(j)->GetMaterial()->diffuseSrvPtr);
			if (!result) LOG_WARNING << "Error setting shader resource view " << "diffuseTexture" << " to pixel shader. Variable not found." << std::endl;
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetShaderResourceView("normalTexture", entities[i]->GetMeshAt(j)->GetMaterial()->normalSrvPtr);
			if (!result) LOG_WARNING << "Error setting shader resource view " << "normalTexture" << " to pixel shader. Variable not found." << std::endl;

			// Set All IBL data
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetShaderResourceView("cubemap", skyboxes[currentSkybox]->GetCubemapSrv());
			if (!result) LOG_WARNING << "Error setting shader resource view " << "cubemap" << " to pixel shader. Variable not found." << std::endl;
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetShaderResourceView("irradianceMap", skyboxes[currentSkybox]->GetIrradianceSrv());
			if (!result) LOG_WARNING << "Error setting shader resource view " << "irradianceMap" << " to pixel shader. Variable not found." << std::endl;
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetShaderResourceView("shadowMap", lights[0]->GetShadowResourceView());
			if (!result) LOG_WARNING << "Error setting shader resource view " << "shadowMap" << " to pixel shader. Variable not found." << std::endl;

			// Once you've set all of the data you care to change for
			// the next draw call, you need to actually send it to the GPU
			//  - If you skip this, the "SetMatrix" calls above won't make it to the GPU!
			entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->CopyAllBufferData();
			entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->CopyAllBufferData();

			// Set the vertex and pixel shaders to use for the next Draw() command
			//  - These don't technically need to be set every frame...YET
			//  - Once you start applying different shaders to different objects,
			//    you'll need to swap the current shaders before each draw
			entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetShader();
			entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetShader();

			ID3D11Buffer * vertexBuffer = entities[i]->GetMeshAt(j)->GetVertexBuffer();
			ID3D11Buffer * indexBuffer = entities[i]->GetMeshAt(j)->GetIndexBuffer();
			// Set buffers in the input assembler
			//  - Do this ONCE PER OBJECT you're drawing, since each object might
			//    have different geometry.
			UINT stride = sizeof(Vertex);
			UINT offset = 0;
			context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
			context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

			// Finally do the actual drawing
			//  - Do this ONCE PER OBJECT you intend to draw
			//  - This will use all of the currently set DirectX "stuff" (shaders, buffers, etc)
			//  - DrawIndexed() uses the currently set INDEX BUFFER to look up corresponding
			//     vertices in the currently set VERTEX BUFFER
			context->DrawIndexed(
				entities[i]->GetMeshAt(j)->GetIndexCount(),		// The number of indices to use (we could draw a subset if we wanted)
				0,								// Offset to the first index we want to use
				0);								// Offset to add to each index when looking up vertices

			// Unbind shadowMap
			entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetShaderResourceView("shadowMap", nullptr);
		}
	}


	// Render Skybox
	XMFLOAT3 camPos = camera->GetPosition();
	XMFLOAT4X4 worldMat{};
	XMFLOAT4X4 viewMat{};
	XMFLOAT4X4 projMat{};
	XMStoreFloat4x4(&viewMat, camera->GetViewMatrix());
	XMStoreFloat4x4(&projMat, camera->GetProjectionMatrix());
	XMMATRIX w = XMMatrixMultiply(XMMatrixRotationQuaternion(skyboxes[currentSkybox]->GetRotationQuaternion()), XMMatrixTranslation(camPos.x, camPos.y, camPos.z));
	XMStoreFloat4x4(&worldMat, XMMatrixTranspose(w));

	bool result;
	result = skyboxes[currentSkybox]->GetVertexShader()->SetMatrix4x4("world", worldMat);
	if (!result) LOG_WARNING << "Error setting parameter " << "world" << " to skybox vertex shader. Variable not found." << std::endl;

	result = skyboxes[currentSkybox]->GetVertexShader()->SetMatrix4x4("view", viewMat);
	if (!result) LOG_WARNING << "Error setting parameter " << "view" << " to skybox vertex shader. Variable not found." << std::endl;

	result = skyboxes[currentSkybox]->GetVertexShader()->SetMatrix4x4("projection", projMat);
	if (!result) LOG_WARNING << "Error setting parameter " << "projection" << " to vertex skybox shader. Variable not found." << std::endl;

	// Sampler and Texture
	result = skyboxes[currentSkybox]->GetPixelShader()->SetSamplerState("basicSampler", skyboxes[currentSkybox]->GetSamplerState());
	if (!result) LOG_WARNING << "Error setting sampler state " << "basicSampler" << " to skybox pixel shader. Variable not found." << std::endl;
	result = skyboxes[currentSkybox]->GetPixelShader()->SetShaderResourceView("cubemapTexture", skyboxes[currentSkybox]->GetCubemapSrv());
	if (!result) LOG_WARNING << "Error setting shader resource view " << "cubemapTexture" << " to skybox pixel shader. Variable not found." << std::endl;

	skyboxes[currentSkybox]->GetVertexShader()->CopyAllBufferData();
	skyboxes[currentSkybox]->GetPixelShader()->CopyAllBufferData();

	skyboxes[currentSkybox]->GetVertexShader()->SetShader();
	skyboxes[currentSkybox]->GetPixelShader()->SetShader();

	ID3D11Buffer * vertexBuffer = skyboxes[currentSkybox]->GetVertexBuffer();
	ID3D11Buffer * indexBuffer = skyboxes[currentSkybox]->GetIndexBuffer();

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	context->DrawIndexed(36, 0, 0);
#pragma endregion 
#pragma region PostProcessing
#pragma endregion 


	// Present the back buffer to the user
	//  - Puts the final frame we're drawing into the window so the user can see it
	//  - Do this exactly ONCE PER FRAME (always at the very end of the frame)
	swapChain->Present(0, 0);
}


#pragma region Mouse Input

// --------------------------------------------------------
// Helper method for mouse clicking.  We get this information
// from the OS-level messages anyway, so these helpers have
// been created to provide basic mouse input if you want it.
// --------------------------------------------------------
void Game::OnMouseDown(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...

	// Save the previous mouse position, so we have it for the future
	prevMousePos.x = x;
	prevMousePos.y = y;

	// Capture the mouse so we keep getting mouse move
	// events even if the mouse leaves the window.  we'll be
	// releasing the capture once a mouse button is released
	SetCapture(hWnd);
}

// --------------------------------------------------------
// Helper method for mouse release
// --------------------------------------------------------
void Game::OnMouseUp(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...

	// We don't care about the tracking the cursor outside
	// the window anymore (we're not dragging if the mouse is up)
	ReleaseCapture();
}

// --------------------------------------------------------
// Helper method for mouse movement.  We only get this message
// if the mouse is currently over the window, or if we're 
// currently capturing the mouse.
// --------------------------------------------------------
void Game::OnMouseMove(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...
	if (buttonState& MK_LBUTTON)
	{
		const LONG deltaX = x - prevMousePos.x;
		const LONG deltaY = y - prevMousePos.y;
		camera->Update(0.0f, 0.0f, 0.0f, deltaX * 0.001f, deltaY * 0.001f);
	}

	// Save the previous mouse position, so we have it for the future
	prevMousePos.x = x;
	prevMousePos.y = y;
}

// --------------------------------------------------------
// Helper method for mouse wheel scrolling.  
// WheelDelta may be positive or negative, depending 
// on the direction of the scroll
// --------------------------------------------------------
void Game::OnMouseWheel(float wheelDelta, int x, int y)
{
	// Add any custom code here...
	float scale = 1.0f + wheelDelta / 3.0f;
	if (scale < 0) scale = 1.0f;

	for (int i = 0; i < entityCount; ++i)
	{
		XMFLOAT3 objTranslation = entities[i]->GetTranslation();
		XMFLOAT3 objScale = entities[i]->GetScale();

		XMVECTOR vec = XMLoadFloat3(&objTranslation);
		vec = XMVectorScale(vec, scale);
		XMStoreFloat3(&objTranslation, vec);

		vec = XMLoadFloat3(&objScale);
		vec = XMVectorScale(vec, scale);
		XMStoreFloat3(&objScale, vec);

		entities[i]->SetTranslation(objTranslation);
		entities[i]->SetScale(objScale);
	}
}
#pragma endregion
