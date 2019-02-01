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

#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
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

	if (anisotropicSampler) { anisotropicSampler->Release(); }
	if (preIntegratedSrv) { preIntegratedSrv->Release(); }

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

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	CreateMatrices();
	CreateBasicGeometry();

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
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files using
// my SimpleShader wrapper for DirectX shader manipulation.
// - SimpleShader provides helpful methods for sending
//   data to individual variables on the GPU
// --------------------------------------------------------
void Game::LoadShaders()
{
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

	// Load Pre Integrated Texture
	// texture file
	std::string name = "model\\ibl_brdf_lut.png";
	// string -> wstring
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
	std::wstring wName = cv.from_bytes(name);
	HRESULT hr = DirectX::CreateWICTextureFromFile(device, context, wName.c_str(), nullptr, &preIntegratedSrv);
	if (FAILED(hr))
	{
		LOG_WARNING << "Failed to load pre-integrated texture file \"" << name << "\"." << std::endl;
	}
	else
	{
		LOG_INFO << "Load pre-integrated texture file \"" << name << "\"." << std::endl;
	}

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));

	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = device->CreateSamplerState(&samplerDesc, &anisotropicSampler);
	if (FAILED(hr))
		LOG_ERROR << "Failed to create anisotropic sampler." << std::endl;

	BlinnPhongMaterial::GetDefault()->SetVertexShaderPtr(vertexShader);
	BlinnPhongMaterial::GetDefault()->SetPixelShaderPtr(blinnPhongPixelShader);

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
	lightCount = 1;
	lights = new Light[lightCount];

	//lights[0] = DirectionalLight(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 1.0f, 0.0f), 1.0f, XMFLOAT3(0.0f, 0.0f, 0.0f));
	//lights[1] = PointLight(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(2.0f, 0.0f, 0.0f), 3.0f, 1.0f);
	//lights[2] = SpotLight(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -2.0f), XMFLOAT3(0.0f, 0.5f, 1.0f), 10.0f, 0.8f, 1.0f);
	lights[0] = DirectionalLight(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, 0.0f), 1.0f, XMFLOAT3(1.0f, 1.0f, 1.0f));
	//lights[1] = PointLight(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(2.0f, 0.0f, 0.0f), 3.0f, 1.0f);
	//lights[2] = SpotLight(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -2.0f), XMFLOAT3(0.0f, 0.5f, 1.0f), 10.0f, 0.8f, 1.0f);
}



// --------------------------------------------------------
// Initializes the matrices necessary to represent our geometry's 
// transformations and our 3D camera
// --------------------------------------------------------
void Game::CreateMatrices()
{
	// Create FirstPersonCamera
	camera = new FirstPersonCamera(float(width), float(height));
}

// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateBasicGeometry()
{
	entityCount = 121;
	entities = new GameEntity * [entityCount];

	// Create GameEntity & Initial Transform
	const auto modelData1 = Mesh::LoadFromFile("models\\Rock\\sphere.obj", device, context);

	entities[0] = new GameEntity(modelData1.first);
	entities[0]->SetScale(XMFLOAT3(1.0f, 1.0f, 1.0f));

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
			brdfMaterial->InitializeSampler();
		}
		if (originalMaterial->normalSrvPtr)
		{
			originalMaterial->normalSrvPtr->GetResource(&resource);
			originalMaterial->normalSrvPtr->GetDesc(&srvDesc);
			device->CreateShaderResourceView(resource, &srvDesc, &brdfMaterial->normalSrvPtr);
			resource->Release();
			brdfMaterial->InitializeSampler();
		}
		entities[0]->GetMeshAt(k)->SetMaterial(brdfMaterial);
	}
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
bool rotateSkybox = false;
// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	if (animateLight)
	{
		XMFLOAT3 yAxis = { 0.0f, 1.0f, 0.0f };
		XMVECTOR yVec = XMLoadFloat3(&yAxis);
		XMVECTOR rotateQ = XMQuaternionRotationAxis(yVec, deltaTime);
		XMVECTOR lightDirection = XMLoadFloat3(&lights[0].Direction);
		lightDirection = XMVector3Rotate(lightDirection, rotateQ);
		XMStoreFloat3(&lights[0].Direction, lightDirection);

		XMVECTOR lightPosition = XMLoadFloat3(&lights[1].Position);
		lightPosition = XMVector3Rotate(lightPosition, rotateQ);
		XMStoreFloat3(&lights[1].Position, lightPosition);

		XMFLOAT3 xAxis = { 1.0f, 0.0f, 0.0f };
		XMVECTOR xVec = XMLoadFloat3(&xAxis);
		XMVECTOR rotateXQ = XMQuaternionRotationAxis(xVec, deltaTime);
		XMVECTOR spotLightDirection = XMLoadFloat3(&lights[2].Direction);
		spotLightDirection = XMVector3Rotate(spotLightDirection, rotateXQ);
		XMStoreFloat3(&lights[2].Direction, spotLightDirection);
	}

	if (animateModel)
	{
		XMFLOAT3 yAxis = { 0.0f, 1.0f, 0.0f };
		XMVECTOR yVec = XMLoadFloat3(&yAxis);
		XMVECTOR rotateQ = XMQuaternionRotationAxis(yVec, deltaTime);
		XMVECTOR modelRotation = XMLoadFloat4(&entities[0]->GetRotation());
		modelRotation = XMQuaternionMultiply(modelRotation, rotateQ);
		XMFLOAT4 rot{};
		XMStoreFloat4(&rot, modelRotation);
		entities[0]->SetRotation(rot);
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

	// Change Skybox
	if (GetAsyncKeyState('K') & 0x1)
	{
		currentSkybox += 1;
		currentSkybox %= skyboxCount;
	}

	// Set Roughness
	if ((GetAsyncKeyState('1') & 0x8000) && (GetAsyncKeyState(VK_UP) & 0x1))
	{
		for (int i = 0; i < entityCount; ++i)
		{
			for (int j = 0; j < entities[i]->GetMeshCount(); ++j)
			{
				BrdfMaterial* material = reinterpret_cast<BrdfMaterial*>(entities[i]->GetMeshAt(j)->GetMaterial());
				material->parameters.roughness += 0.1f;
				if (material->parameters.roughness > 1.0f) material->parameters.roughness = 1.0f;
			}
		}
	}
	if ((GetAsyncKeyState('1') & 0x8000) && (GetAsyncKeyState(VK_DOWN) & 0x1))
	{
		for (int i = 0; i < entityCount; ++i)
		{
			for (int j = 0; j < entities[i]->GetMeshCount(); ++j)
			{
				BrdfMaterial* material = reinterpret_cast<BrdfMaterial*>(entities[i]->GetMeshAt(j)->GetMaterial());
				material->parameters.roughness -= 0.1f;
				if (material->parameters.roughness < 0.0f) material->parameters.roughness = 0.0f;
			}
		}
	}
	// Set Metalness
	if ((GetAsyncKeyState('2') & 0x8000) && (GetAsyncKeyState(VK_UP) & 0x1))
	{
		for (int i = 0; i < entityCount; ++i)
		{
			for (int j = 0; j < entities[i]->GetMeshCount(); ++j)
			{
				BrdfMaterial* material = reinterpret_cast<BrdfMaterial*>(entities[i]->GetMeshAt(j)->GetMaterial());
				material->parameters.metalness += 0.1f;
				if (material->parameters.metalness > 1.0f) material->parameters.metalness = 1.0f;
			}
		}
	}
	if ((GetAsyncKeyState('2') & 0x8000) && (GetAsyncKeyState(VK_DOWN) & 0x1))
	{
		for (int i = 0; i < entityCount; ++i)
		{
			for (int j = 0; j < entities[i]->GetMeshCount(); ++j)
			{
				BrdfMaterial* material = reinterpret_cast<BrdfMaterial*>(entities[i]->GetMeshAt(j)->GetMaterial());
				material->parameters.metalness -= 0.1f;
				if (material->parameters.metalness < 0.0f) material->parameters.metalness = 0.0f;
			}
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
	// Background color (Cornflower Blue in this case) for clearing
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

	// Update camera view matrix before drawing
	camera->UpdateViewMatrix();

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
			bool result;
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("world", entities[i]->GetWorldMatrix());
			if (!result) LOG_WARNING << "Error setting parameter " << "world" << " to vertex shader. Variable not found." << std::endl;

			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("itworld", entities[i]->GetWorldMatrixIT());
			if (!result) LOG_WARNING << "Error setting parameter " << "itworld" << " to vertex shader. Variable not found." << std::endl;

			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("view", viewMat);
			if (!result) LOG_WARNING << "Error setting parameter " << "view" << " to vertex shader. Variable not found." << std::endl;

			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("projection", projMat);
			if (!result) LOG_WARNING << "Error setting parameter " << "projection" << " to vertex shader. Variable not found." << std::endl;


			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetInt("lightCount", lightCount);
			if (!result) LOG_WARNING << "Error setting parameter " << "lightCount" << " to pixel shader. Variable not found." << std::endl;


			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetData(
				"lights",					// The name of the (eventual) variable in the shader
				lights,							// The address of the data to copy
				sizeof(Light) * 128);		// The size of the data to copy
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

			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetFloat3("CameraDirection", camera->GetForward());
			if (!result) LOG_WARNING << "Error setting parameter " << "CameraDirection" << " to pixel shader. Variable not found or size incorrect." << std::endl;

			XMMATRIX skyboxRotationMatrix = XMMatrixTranspose(XMMatrixRotationQuaternion(XMQuaternionInverse(skyboxes[currentSkybox]->GetRotationQuaternion())));
			XMFLOAT4X4 m{};
			XMStoreFloat4x4(&m, skyboxRotationMatrix);
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetMatrix4x4("SkyboxRotation", m);
			if (!result) LOG_WARNING << "Error setting parameter " << "SkyboxRotation" << " to pixel shader. Variable not found or size incorrect." << std::endl;
			// Sampler and Texture
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetSamplerState("basicSampler", entities[i]->GetMeshAt(j)->GetMaterial()->GetSamplerState());
			if (!result) LOG_WARNING << "Error setting sampler state " << "basicSampler" << " to pixel shader. Variable not found." << std::endl;
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetShaderResourceView("diffuseTexture", entities[i]->GetMeshAt(j)->GetMaterial()->diffuseSrvPtr);
			if (!result) LOG_WARNING << "Error setting shader resource view " << "diffuseTexture" << " to pixel shader. Variable not found." << std::endl;
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetShaderResourceView("normalTexture", entities[i]->GetMeshAt(j)->GetMaterial()->normalSrvPtr);
			if (!result) LOG_WARNING << "Error setting shader resource view " << "normalTexture" << " to pixel shader. Variable not found." << std::endl;

			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetShaderResourceView("cubemap", skyboxes[currentSkybox]->GetCubemapSrv());
			if (!result) LOG_WARNING << "Error setting shader resource view " << "cubemap" << " to pixel shader. Variable not found." << std::endl;
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetShaderResourceView("irradianceMap", skyboxes[currentSkybox]->GetIrradianceSrv());
			if (!result) LOG_WARNING << "Error setting shader resource view " << "irradianceMap" << " to pixel shader. Variable not found." << std::endl;

			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetSamplerState("anisotropic", anisotropicSampler);
			if (!result) LOG_WARNING << "Error setting sampler state " << "anisotropic" << " to pixel shader. Variable not found." << std::endl;
			result = entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetShaderResourceView("preIntegrated", preIntegratedSrv);
			if (!result) LOG_WARNING << "Error setting shader resource view " << "preIntegrated" << " to pixel shader. Variable not found." << std::endl;
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
		const float zTemp = objTranslation.z;
		XMFLOAT3 objScale = entities[i]->GetScale();

		XMVECTOR vec = XMLoadFloat3(&objTranslation);
		vec = XMVectorScale(vec, scale);
		XMStoreFloat3(&objTranslation, vec);
		objTranslation.z = zTemp;

		vec = XMLoadFloat3(&objScale);
		vec = XMVectorScale(vec, scale);
		XMStoreFloat3(&objScale, vec);

		entities[i]->SetTranslation(objTranslation);
		entities[i]->SetScale(objScale);
	}
}
#pragma endregion
