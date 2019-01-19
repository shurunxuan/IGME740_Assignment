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
	//delete vertexShader;
	//delete pixelShader;

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
}

#pragma region Icosphere
struct Triangle
{
	int vertex[3];
};

namespace icosahedron
{
	const float X = .525731112119133606f;
	const float Z = .850650808352039932f;
	const float N = 0.f;

	static const std::vector<XMFLOAT3> vertices =
	{
		{-X, N, Z}, {X, N, Z}, {-X, N, -Z}, {X, N, -Z},
		{N, Z, X}, {N, Z, -X}, {N, -Z, X}, {N, -Z, -X},
		{Z, X, N}, {-Z, X, N}, {Z, -X, N}, {-Z, -X, N}
	};

	static const std::vector<Triangle> triangles =
	{
		{0, 4, 1}, {0, 9, 4}, {9, 5, 4}, {4, 5, 8}, {4, 8, 1},
		{8, 10, 1}, {8, 3, 10}, {5, 3, 8}, {5, 2, 3}, {2, 7, 3},
		{7, 10, 3}, {7, 6, 10}, {7, 11, 6}, {11, 0, 6}, {0, 1, 6},
		{6, 1, 10}, {9, 0, 11}, {9, 11, 2}, {9, 2, 5}, {7, 2, 11}
	};
}

std::pair<std::vector<XMFLOAT3>, std::vector<Triangle>> MakeSphere(int subdivisions)
{
	std::vector<XMFLOAT3> vertices = icosahedron::vertices;
	std::vector<Triangle> triangles = icosahedron::triangles;

	for (int i = 0; i < subdivisions; ++i)
	{
		std::map<std::pair<int, int>, int> lookup;
		std::vector<Triangle> result;
		result.reserve(int(pow(4, i)) * 80);

		for (Triangle& triangle : triangles)
		{
			std::array<int, 3> mid{};
			for (int vtx = 0; vtx < 3; ++vtx)
			{
				//mid[edge] = VertexForEdge(lookup, vertices,
				//	triangle.vertex[edge], triangle.vertex[(edge + 1) % 3]);
				int v1 = triangle.vertex[vtx];
				int v2 = triangle.vertex[(vtx + 1) % 3];
				std::pair<int, int> key(v1, v2);
				if (key.first > key.second)
					std::swap(key.first, key.second);

				const std::pair<std::map<std::pair<int, int>, int>::iterator, bool> inserted = lookup.insert({
					key, int(vertices.size())
					});
				if (inserted.second)
				{
					XMFLOAT3& vertex0 = vertices[v1];
					XMFLOAT3& vertex1 = vertices[v2];
					const XMVECTOR midVertex = XMVector3Normalize(XMLoadFloat3(&vertex0) + XMLoadFloat3(&vertex1));
					XMFLOAT3 v{};
					XMStoreFloat3(&v, midVertex);
					vertices.push_back(v);
				}

				mid[vtx] = inserted.first->second;
			}

			result.push_back({ triangle.vertex[0], mid[0], mid[2] });
			result.push_back({ triangle.vertex[1], mid[1], mid[0] });
			result.push_back({ triangle.vertex[2], mid[2], mid[1] });
			result.push_back({ mid[0], mid[1], mid[2] });
		}

		triangles = result;
	}

	return{ vertices, triangles };
}

#pragma endregion 

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
	const std::pair<std::vector<XMFLOAT3>, std::vector<Triangle>> sphere =
		MakeSphere(4);
	std::vector<XMFLOAT3> positions = sphere.first;
	entityCount = int(positions.size());
	entities = new GameEntity*[entityCount];

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

	// Create Material
	const std::shared_ptr<Material> unlitMaterial = std::make_shared<Material>(vertexShader, pixelShader);

	// Create GameEntity
	// Initial Transform
	for (int i = 0; i < entityCount; ++i)
	{
		entities[i] = new GameEntity(meshCube, unlitMaterial);
		entities[i]->SetScale(XMFLOAT3(0.02f, 0.02f, 0.02f));

		XMVECTOR pos = XMLoadFloat3(&*(positions.begin() + i));
		pos = XMVectorScale(pos, 3.0f);
		XMFLOAT3 t{};
		XMStoreFloat3(&t, pos);
		entities[i]->SetTranslation(t);
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
	XMFLOAT3 axis_0 = { 1.0f, 1.0f, -1.0f };
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
		// Send data to shader variables
		//  - Do this ONCE PER OBJECT you're drawing
		//  - This is actually a complex process of copying data to a local buffer
		//    and then copying that entire buffer to the GPU.  
		//  - The "SimpleShader" class handles all of that for you.
		XMFLOAT4X4 viewMat{};
		XMFLOAT4X4 projMat{};
		XMStoreFloat4x4(&viewMat, camera->GetViewMatrix());
		XMStoreFloat4x4(&projMat, camera->GetProjectionMatrix());
		entities[i]->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("world", entities[i]->GetWorldMatrix());
		entities[i]->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("view", viewMat);
		entities[i]->GetMaterial()->GetVertexShaderPtr()->SetMatrix4x4("projection", projMat);

		// Once you've set all of the data you care to change for
		// the next draw call, you need to actually send it to the GPU
		//  - If you skip this, the "SetMatrix" calls above won't make it to the GPU!
		entities[i]->GetMaterial()->GetVertexShaderPtr()->CopyAllBufferData();

		// Set the vertex and pixel shaders to use for the next Draw() command
		//  - These don't technically need to be set every frame...YET
		//  - Once you start applying different shaders to different objects,
		//    you'll need to swap the current shaders before each draw
		entities[i]->GetMaterial()->GetVertexShaderPtr()->SetShader();
		entities[i]->GetMaterial()->GetPixelShaderPtr()->SetShader();


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

}
#pragma endregion