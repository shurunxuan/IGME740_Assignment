#include "Game.h"
#include "Vertex.h"

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
	vertexShader = 0;
	pixelShader = 0;

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

	// Delete our simple shader objects, which
	// will clean up their own internal DirectX stuff
	delete vertexShader;
	delete pixelShader;

	// Delete GameEntity data
	for (int i = 0; i < entityCount; ++i)
	{
		// TODO: Potential Access Violation not handled.
		delete entities[i];
	}
	delete[] entities;

	// Delete Camera
	delete camera;
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
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

	pixelShader = new SimplePixelShader(device, context);
	pixelShader->LoadShaderFile(L"PixelShader.cso");
}



// --------------------------------------------------------
// Initializes the matrices necessary to represent our geometry's 
// transformations and our 3D camera
// --------------------------------------------------------
void Game::CreateMatrices()
{
	// Create FirstPersonCamera
	camera = new FirstPersonCamera(float(width), float(height));
	//// Set up world matrix
	//// - In an actual game, each object will need one of these and they should
	////    update when/if the object moves (every frame)
	//// - You'll notice a "transpose" happening below, which is redundant for
	////    an identity matrix.  This is just to show that HLSL expects a different
	////    matrix (column major vs row major) than the DirectX Math library
	//XMMATRIX W = XMMatrixIdentity();
	//XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(W)); // Transpose for HLSL!

	//// Create the View matrix
	//// - In an actual game, recreate this matrix every time the camera 
	////    moves (potentially every frame)
	//// - We're using the LOOK TO function, which takes the position of the
	////    camera and the direction vector along which to look (as well as "up")
	//// - Another option is the LOOK AT function, to look towards a specific
	////    point in 3D space
	//XMVECTOR pos = XMVectorSet(0, 0, -5, 0);
	//XMVECTOR dir = XMVectorSet(0, 0, 1, 0);
	//XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	//XMMATRIX V = XMMatrixLookToLH(
	//	pos,     // The position of the "camera"
	//	dir,     // Direction the camera is looking
	//	up);     // "Up" direction in 3D space (prevents roll)
	//XMStoreFloat4x4(&viewMatrix, XMMatrixTranspose(V)); // Transpose for HLSL!

	//// Create the Projection matrix
	//// - This should match the window's aspect ratio, and also update anytime
	////    the window resizes (which is already happening in OnResize() below)
	//XMMATRIX P = XMMatrixPerspectiveFovLH(
	//	0.25f * 3.1415926535f,		// Field of View Angle
	//	(float)width / height,		// Aspect ratio
	//	0.1f,						// Near clip plane distance
	//	100.0f);					// Far clip plane distance
	//XMStoreFloat4x4(&projectionMatrix, XMMatrixTranspose(P)); // Transpose for HLSL!
}

// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateBasicGeometry()
{
	// Create some temporary variables to represent colors
	// - Not necessary, just makes things more readable
	const XMFLOAT4 red = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	const XMFLOAT4 green = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	const XMFLOAT4 blue = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	const XMFLOAT4 yellow = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
	const XMFLOAT4 cyan = XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	const XMFLOAT4 magenta = XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f);
	const XMFLOAT4 white = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	const XMFLOAT4 black = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	// Create GameEntity data
	entityCount = 5;
	entities = new GameEntity*[entityCount];

	// Create a triangle mesh
	Vertex verticesTri[] =
	{
		{ XMFLOAT3(+0.0f, +1.0f, +0.0f), red },
		{ XMFLOAT3(+0.866f, -0.5f, +0.0f), blue },
		{ XMFLOAT3(-0.866f, -0.5f, +0.0f), green },
	};

	int indicesTri[] = { 0, 1, 2 };

	const std::shared_ptr<Mesh> meshTri = std::make_shared<Mesh>(verticesTri, 3, indicesTri, 3, device);

	// Create a cube mesh
	Vertex verticesCube[] =
	{
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), white },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), red },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), yellow },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), green },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), cyan },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), blue },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), magenta },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), black },
	};

	int indicesCube[] = {
		0, 1, 6, 0, 6, 5,
		0, 5, 4, 0, 4, 3,
		0, 3, 2, 0, 2, 1,
		7, 6, 1, 7, 1, 2,
		7, 2, 3, 7, 3, 4,
		7, 4, 5, 7, 5, 6,
	};
	const std::shared_ptr<Mesh> meshCube = std::make_shared<Mesh>(verticesCube, 8, indicesCube, 36, device);

	// Create a circle mesh
	const int slices = 1000;
	Vertex* verticesCir = new Vertex[slices + 1];
	int* indicesCir = new int[slices * 3];

	// Center of the circle
	verticesCir[0].Position = XMFLOAT3(+0.0f, +0.0f, +0.0f);
	verticesCir[0].Color = white;
	for (int i = 0; i < slices; ++i)
	{
		verticesCir[i + 1].Position = XMFLOAT3(-sin(float(i) / slices * 2 * 3.1415927f), +cos(float(i) / slices * 2 * 3.1415927f), +0.0f);

		// Convert HSL color space to RGB to make a color wheel
		XMFLOAT4 hsvColor = { float(i) / float(slices), 1.0f, 1.0f, 1.0f };
		const XMVECTOR hsvColorVector = XMLoadFloat4(&hsvColor);
		const XMVECTOR rgbColorVector = XMColorHSVToRGB(hsvColorVector);
		XMStoreFloat4(&verticesCir[i + 1].Color, rgbColorVector);

		// The order
		indicesCir[3 * i] = i == slices - 1 ? 1 : i + 2;
		indicesCir[3 * i + 1] = i + 1;
		indicesCir[3 * i + 2] = 0;
	}

	const std::shared_ptr<Mesh> meshCir = std::make_shared<Mesh>(verticesCir, slices + 1, indicesCir, slices * 3, device);

	// Clear the memory
	delete[] verticesCir;
	delete[] indicesCir;

	// Create GameEntity
	entities[0] = new GameEntity(meshCube);
	entities[1] = new GameEntity(meshTri);
	entities[2] = new GameEntity(meshCir);
	entities[3] = new GameEntity(meshTri);
	entities[4] = new GameEntity(meshCir);

	// Initial Transform
	entities[1]->SetTranslation(XMFLOAT3(+1.0f, +1.0f, +0.0f));
	entities[2]->SetTranslation(XMFLOAT3(-1.0f, +1.0f, +0.0f));
	entities[3]->SetTranslation(XMFLOAT3(+1.0f, -1.0f, +0.0f));
	entities[4]->SetTranslation(XMFLOAT3(-1.0f, -1.0f, +0.0f));

	for (int i = 0; i < entityCount; ++i)
	{
		entities[i]->SetScale(XMFLOAT3(0.2f, 0.2f, 0.2f));
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

	camera->UpdateProjectionMatrix(float(width), float(height), 3.14159265f / 3.0f);
	//// Update our projection matrix since the window size changed
	//XMMATRIX P = XMMatrixPerspectiveFovLH(
	//	0.25f * 3.1415926535f,	// Field of View Angle
	//	(float)width / height,	// Aspect ratio
	//	0.1f,				  	// Near clip plane distance
	//	100.0f);			  	// Far clip plane distance
	//XMStoreFloat4x4(&projectionMatrix, XMMatrixTranspose(P)); // Transpose for HLSL!
}

// Several small variables to record the direction of the animation
bool animationDirection = true;

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Rotate the Cube Mesh (0)
	XMVECTOR rQua_0 = XMLoadFloat4(&entities[0]->GetRotation());
	XMFLOAT3 axis_0 = { 1.0f, 1.0f, -1.0f };
	XMVECTOR newR_0 = XMQuaternionRotationAxis(XMLoadFloat3(&axis_0), deltaTime * 2.0f);
	rQua_0 = XMQuaternionMultiply(rQua_0, newR_0);
	XMFLOAT4 r_0;
	XMStoreFloat4(&r_0, rQua_0);
	entities[0]->SetRotation(r_0);

	// Rotate the Triangle Mesh (1) 
	XMVECTOR rQua_1 = XMLoadFloat4(&entities[1]->GetRotation());
	XMFLOAT3 axis_1 = { 0.0f, 0.0f, 1.0f };
	XMVECTOR newR_1 = XMQuaternionRotationAxis(XMLoadFloat3(&axis_1), deltaTime * 2.0f);
	rQua_1 = XMQuaternionMultiply(rQua_1, newR_1);
	XMFLOAT4 r_1;
	XMStoreFloat4(&r_1, rQua_1);
	entities[1]->SetRotation(r_1);

	// Move the Circle Mesh (2) in a circle
	XMFLOAT3 t_2 = { -1.0f + cos(totalTime / 3.0f * 3.1415927f) / 3.0f, 1.0f + sin(totalTime / 3.0f * 3.1415927f) / 3.0f, 0.0f };
	entities[2]->SetTranslation(t_2);

	// Rotate the Circle Mesh (2) 
	XMVECTOR rQua_2 = XMLoadFloat4(&entities[2]->GetRotation());
	XMFLOAT3 axis_2 = { 0.0f, 0.0f, 1.0f };
	XMVECTOR newR_2 = XMQuaternionRotationAxis(XMLoadFloat3(&axis_2), deltaTime * 8.0f);
	rQua_2 = XMQuaternionMultiply(rQua_2, newR_2);
	XMFLOAT4 r_2;
	XMStoreFloat4(&r_2, rQua_2);
	entities[2]->SetRotation(r_2);

	// Move the Circle Mesh (4)
	XMVECTOR tVec_4 = XMLoadFloat3(&entities[4]->GetTranslation());
	XMFLOAT3 tDir(0.5f * deltaTime * (animationDirection ? 1 : -1), 0.0f, 0.0f);
	XMVECTOR tDirVec = XMLoadFloat3(&tDir);
	XMVECTOR newTVec = XMVectorAdd(tVec_4, tDirVec);
	XMFLOAT3 newT;
	XMStoreFloat3(&newT, newTVec);
	if (newT.x > -0.5f || newT.x < -1.5f) animationDirection = !animationDirection;
	entities[4]->SetTranslation(newT);

	// Rotate the Circle Mesh(4)
	XMVECTOR rQua_4 = XMLoadFloat4(&entities[4]->GetRotation());
	XMFLOAT3 axis_4 = { 0.0f, 0.0f, 1.0f };
	XMVECTOR newR_4 = XMQuaternionRotationAxis(XMLoadFloat3(&axis_4), deltaTime * 4.0f);
	rQua_4 = XMQuaternionMultiply(rQua_4, newR_4);
	XMFLOAT4 r_4;
	XMStoreFloat4(&r_4, rQua_4);
	entities[4]->SetRotation(r_4);

	// WASD for moving camera
	XMFLOAT3 forward = camera->GetForward();
	XMFLOAT3 right = camera->GetRight();
	XMFLOAT3 up(0.0f, 1.0f, 0.0f);
	const float speed = 2.0f * deltaTime;
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
	const float color[4] = { 0.4f, 0.6f, 0.75f, 0.0f };

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
		// Send data to shader variables
		//  - Do this ONCE PER OBJECT you're drawing
		//  - This is actually a complex process of copying data to a local buffer
		//    and then copying that entire buffer to the GPU.  
		//  - The "SimpleShader" class handles all of that for you.
		XMFLOAT4X4 viewMat{};
		XMFLOAT4X4 projMat{};
		XMStoreFloat4x4(&viewMat, camera->GetViewMatrix());
		XMStoreFloat4x4(&projMat, camera->GetProjectionMatrix());
		vertexShader->SetMatrix4x4("world", entities[i]->GetWorldMatrix());
		vertexShader->SetMatrix4x4("view", viewMat);
		vertexShader->SetMatrix4x4("projection", projMat);

		// Once you've set all of the data you care to change for
		// the next draw call, you need to actually send it to the GPU
		//  - If you skip this, the "SetMatrix" calls above won't make it to the GPU!
		vertexShader->CopyAllBufferData();

		// Set the vertex and pixel shaders to use for the next Draw() command
		//  - These don't technically need to be set every frame...YET
		//  - Once you start applying different shaders to different objects,
		//    you'll need to swap the current shaders before each draw
		vertexShader->SetShader();
		pixelShader->SetShader();


		ID3D11Buffer* vertexBuffer = entities[i]->GetMesh()->GetVertexBuffer();
		ID3D11Buffer* indexBuffer = entities[i]->GetMesh()->GetIndexBuffer();
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
			entities[i]->GetMesh()->GetIndexCount(),		// The number of indices to use (we could draw a subset if we wanted)
			0,								// Offset to the first index we want to use
			0);								// Offset to add to each index when looking up vertices
	}



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

	// Caputure the mouse so we keep getting mouse move
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
	const LONG deltaX = x - prevMousePos.x;
	const LONG deltaY = y - prevMousePos.y;
	camera->Update(0.0f, 0.0f, 0.0f, deltaX * 0.001f, deltaY * 0.001f);

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
	XMVECTOR sVec = XMLoadFloat3(&entities[4]->GetScale());
	sVec = XMVectorScale(sVec, 1.0f + wheelDelta / 10.0f);
	XMFLOAT3 s;
	XMStoreFloat3(&s, sVec);
	entities[4]->SetScale(s);

	//XMVECTOR tVec = XMLoadFloat3(&entities[0]->GetTranslation());
	//XMFLOAT3 t = { wheelDelta / 10.0f, wheelDelta / 10.0f, 0.0f };
	//tVec = XMVectorAdd(tVec, XMLoadFloat3(&t));
	//XMStoreFloat3(&t, tVec);
	//entities[0]->SetTranslation(t);

	//XMVECTOR rQua = XMLoadFloat4(&entities[0]->GetRotation());
	//XMFLOAT3 axis = { 1.0f, 1.0f, -1.0f };
	//XMVECTOR newR = XMQuaternionRotationAxis(XMLoadFloat3(&axis), wheelDelta / 10.0f);
	//rQua = XMQuaternionMultiply(rQua, newR);
	//XMFLOAT4 r;
	//XMStoreFloat4(&r, rQua);
	//entities[0]->SetRotation(r);
}
#pragma endregion