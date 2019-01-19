#include "Game.h"
#include "Vertex.h"
#include <map>
#include <array>

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

	Material::GetDefault()->SetVertexShaderPtr(vertexShader);
	Material::GetDefault()->SetPixelShaderPtr(pixelShader);
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
	std::string filename;
	// Open a file
	OPENFILENAME ofn;       // common dialog box structure
	char szFile[260];       // buffer for file name

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "OBJ Model file\0*.obj\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = "Choose a .obj file";
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn) == TRUE)
	{
		filename = szFile;
	}
	else
	{
		filename = "D:\\models\\cone.obj";
		printf("[WARNING] No file chosen. Fallback to default file \"");
		printf(filename.c_str());
		printf("\".\n");
	}

	const auto modelData = Mesh::LoadFromFile(filename, device);

	// The shaders are not set yet so we need to set it now.
	for (const std::shared_ptr<Material>& mat : modelData.second)
	{
		mat->SetVertexShaderPtr(vertexShader);
		mat->SetPixelShaderPtr(pixelShader);
	}

	// Create GameEntity data
	entityCount = 1;
	entities = new GameEntity*[entityCount];

	// Create GameEntity
	// Initial Transform
	for (int i = 0; i < entityCount; ++i)
	{
		entities[i] = new GameEntity(modelData.first);
		entities[i]->SetTranslation(XMFLOAT3(0.0f, 0.0f, 5.0f));
		entities[i]->SetScale(XMFLOAT3(0.5f, 0.5f, 0.5f));
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
bool animationDirection = true;

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Rotate the Cube Meshes
	XMVECTOR rQua_0 = XMLoadFloat4(&entities[0]->GetRotation());
	XMFLOAT3 axis_0 = { 0.0f, 1.0f, 0.0f };
	const XMVECTOR newR_0 = XMQuaternionRotationAxis(XMLoadFloat3(&axis_0), deltaTime * 2.0f);
	rQua_0 = XMQuaternionMultiply(rQua_0, newR_0);
	XMFLOAT4 r_0{};
	XMStoreFloat4(&r_0, rQua_0);
	for (int i = 0; i < entityCount; ++i)
		entities[i]->SetRotation(r_0);

	// W, A, S, D for moving camera
	const XMFLOAT3 forward = camera->GetForward();
	const XMFLOAT3 right = camera->GetRight();
	const XMFLOAT3 up(0.0f, 1.0f, 0.0f);
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
			entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("world", entities[i]->GetWorldMatrix());
			entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("view", viewMat);
			entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("projection", projMat);

			// Once you've set all of the data you care to change for
			// the next draw call, you need to actually send it to the GPU
			//  - If you skip this, the "SetMatrix" calls above won't make it to the GPU!
			entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->CopyAllBufferData();

			// Set the vertex and pixel shaders to use for the next Draw() command
			//  - These don't technically need to be set every frame...YET
			//  - Once you start applying different shaders to different objects,
			//    you'll need to swap the current shaders before each draw
			entities[i]->GetMeshAt(j)->GetMaterial()->GetVertexShaderPtr()->SetShader();
			entities[i]->GetMeshAt(j)->GetMaterial()->GetPixelShaderPtr()->SetShader();


			ID3D11Buffer* vertexBuffer = entities[i]->GetMeshAt(j)->GetVertexBuffer();
			ID3D11Buffer* indexBuffer = entities[i]->GetMeshAt(j)->GetIndexBuffer();
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
	if (buttonState & MK_LBUTTON)
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

}
#pragma endregion