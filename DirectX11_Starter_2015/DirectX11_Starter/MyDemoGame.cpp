// ----------------------------------------------------------------------------
//  A few notes on project settings
//
//  - The project is set to use the UNICODE character set
//    - This was changed in Project Properties > Config Properties > General > Character Set
//    - This basically adds a "#define UNICODE" to the project
//
//  - The include directories were automagically correct, since the DirectX 
//    headers and libs are part of the windows SDK
//    - For instance, $(WindowsSDK_IncludePath) is set as a project include 
//      path by default.  That's where the DirectX headers are located.
//
//  - Two libraries had to be manually added to the Linker Input Dependencies
//    - d3d11.lib
//    - d3dcompiler.lib
//    - This was changed in Project Properties > Config Properties > Linker > Input > Additional Dependencies
//
//  - The Working Directory was changed to match the actual .exe's 
//    output directory, since we need to load the compiled shader files at run time
//    - This was changed in Project Properties > Config Properties > Debugging > Working Directory
//
// ----------------------------------------------------------------------------

#include "MyDemoGame.h"
#include "Vertex.h"

#include <fstream>
#include <iostream>
#include "Logger.h"

// For the DirectX Math library
using namespace DirectX;


#pragma region Win32 Entry Point (WinMain)
// --------------------------------------------------------
// Win32 Entry Point - Where your program starts
// --------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	// Create the game object.
	MyDemoGame game(hInstance);
	
	// This is where we'll create the window, initialize DirectX, 
	// set up geometry and shaders, etc.
	if( !game.Init() )
		return 0;
	
	// All set to run the game loop
	return game.Run();
}

#pragma endregion

#pragma region Constructor / Destructor
// --------------------------------------------------------
// Base class constructor will set up all of the underlying
// fields, and then we can overwrite any that we'd like
// --------------------------------------------------------
MyDemoGame::MyDemoGame(HINSTANCE hInstance) 
	: DirectXGameCore(hInstance)
{
	// Set up a custom caption for the game window.
	// - The "L" before the string signifies a "wide character" string
	// - "Wide" characters take up more space in memory (hence the name)
	// - This allows for an extended character set (more fancy letters/symbols)
	// - Lots of Windows functions want "wide characters", so we use the "L"
	windowCaption = L"My Super Fancy GGP Game";

	// Custom window size - will be created by Init() later
	windowWidth = 1280;
	windowHeight = 720;
}

// --------------------------------------------------------
// Cleans up our DirectX stuff and any objects we need to delete
// - When you make new DX resources, you need to release them here
// - If you don't, you get a lot of scary looking messages in Visual Studio
// --------------------------------------------------------
MyDemoGame::~MyDemoGame()
{
	for (int e = 0; e < ents.size(); e++) {
		delete ents[e];
	}

	for (int m = 0; m < meshes.size(); m++) {
		delete meshes[m];
	}


	delete render;

	// Delete our simple shaders
	delete vertexShader;
	delete pixelShader;

	delete basicMaterial;
}

#pragma endregion

#pragma region Initialization

// --------------------------------------------------------
// Initializes the base class (including the window and D3D),
// sets up our geometry and loads the shaders (among other things)
// --------------------------------------------------------
bool MyDemoGame::Init()
{
	// Call the base class's Init() method to create the window,
	// initialize DirectX, etc.
	if( !DirectXGameCore::Init() )
		return false;

	// Helper methods to create something to draw, load shaders to draw it 
	// with and set up matrices so we can see how to pass data to the GPU.
	//  - For your own projects, feel free to expand/replace these.

	render = new Render(deviceContext);

	LoadShaders(); 
	CreateGeometry();
	TestLoadLevel("Assets/Maps/Untitled.txt");

	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives we'll be using and how to interpret them
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	camera = Camera(0.0f, 0.0f, -5.0f);
	camera.CreatePerspectiveProjectionMatrix(aspectRatio, 0.1f, 100.0f);

	GameLight light1 = GameLight(XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));
	light1.GetTransform().SetRotation(XMFLOAT3(1, -1, 0));
	GameLight light2 = GameLight(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
	light2.GetTransform().SetRotation(XMFLOAT3(1, -1, 0));
	render->SetLight(light1, 0);
	render->SetLight(light2, 1);

	// Successfully initialized
	return true;
}


// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// - These simple shaders provide helpful methods for sending
//   data to individual variables on the GPU
// --------------------------------------------------------
void MyDemoGame::LoadShaders()
{
	vertexShader = new SimpleVertexShader(device, deviceContext);
	vertexShader->LoadShaderFile(L"VertexShader.cso");

	pixelShader = new SimplePixelShader(device, deviceContext);
	pixelShader->LoadShaderFile(L"PixelShader.cso");

	basicMaterial = new Material(vertexShader, pixelShader);
}

void MyDemoGame::TestLoadLevel(char* mapName) {
	std::ifstream s(mapName);
	if (!s.is_open()) {
		return;
	}

	int state = 0;
	//0 is the meta data
	//1 is the turrets
	//2 is light
	//3 is background
	//4 is arena
	char chars[400];
	Entity* currentEntity = nullptr;

	while (s.good())
	{
		s.getline(chars, 400);
		std::string line(chars);
		bool statelessRead = false;
		//LogText(chars);
		if (std::strstr(chars, "arena")) {
			statelessRead = true;
			state = 4;
		}
		else if (std::strstr(chars, "pos")) {
			DirectX::XMFLOAT3 pos;
			sscanf_s(chars, "pos %f %f %f", &pos.x, &pos.y, &pos.z);
			if (currentEntity != nullptr) {
				currentEntity->GetTransform().SetPosition(pos);
			}
			statelessRead = true;
		}
		else if (std::strstr(chars, "rot")) {
			DirectX::XMFLOAT3 rot;
			sscanf_s(chars, "rot %f %f %f", &rot.x, &rot.y, &rot.z);
			if (currentEntity != nullptr) {
				currentEntity->GetTransform().SetRotation(rot);
			}
			statelessRead = true;
		}
		else if (std::strstr(chars, "scl")) {
			DirectX::XMFLOAT3 scl;
			sscanf_s(chars, "scl %f %f %f", &scl.x, &scl.y, &scl.z);
			if (currentEntity != nullptr) {
				currentEntity->GetTransform().SetScale(scl);
			}
			statelessRead = true;
		}
		if (!statelessRead) {
			switch (state)
			{
			case 4:
				if (std::strstr(chars, "model")) 
				{
					std::string modelPath = "Assets/Models/";
					modelPath += line.substr(6, line.length());
					modelPath += ".obj";
					LogText(modelPath);
					Mesh* newMesh = new Mesh(modelPath.c_str(), device);
					currentEntity = new Entity();
					currentEntity->AddComponent(new DrawnMesh(render, newMesh, basicMaterial));
					ents.push_back(currentEntity);
					meshes.push_back(newMesh);
				}
				break;
			default:
				break;
			}
		}
	}
}

// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void MyDemoGame::CreateGeometry()
{
	XMFLOAT3 normal	= XMFLOAT3(0,1, 0);
	XMFLOAT2 uv = XMFLOAT2(0, 0);

	Vertex vertices[] =
	{
		{ XMFLOAT3(-0.5f, +1.0f, +0.0f), normal, uv },// 0
		{ XMFLOAT3(+0.5f, -1.0f, +0.0f), normal, uv },// 1
		{ XMFLOAT3(-0.5f, -1.0f, +0.0f), normal, uv },// 2
		{ XMFLOAT3(+0.5f, +1.0f, 0.0f), normal, uv },// 3
	};
	UINT indices[] = { 0, 1, 2, 0, 3, 1 };
	Mesh* mesh1 = new Mesh("Assets/Models/helix.obj", device);
	//DrawnMesh drawnMesh1 = DrawnMesh(render, mesh1);
	Entity* entity1 = new Entity();
	entity1->AddComponent(new DrawnMesh(render, mesh1, basicMaterial));
	ents.push_back(entity1);
	meshes.push_back(mesh1);

	float halfSize = 100 * 0.5f;
	float yPos = -10.5f;
	Vertex vertices2[] =
	{
		{ XMFLOAT3(-halfSize, +yPos, +halfSize), normal, uv },// 0
		{ XMFLOAT3(+halfSize, +yPos, -halfSize), normal, uv },// 1
		{ XMFLOAT3(-halfSize, +yPos, -halfSize), normal, uv },// 2
		{ XMFLOAT3(+halfSize, +yPos, +halfSize), normal, uv },// 3
	};
	UINT indices2[] = { 0, 1, 2, 0, 3, 1 };
	Mesh* mesh2 = new Mesh(vertices2, 4, indices2, 6, device);
	Entity* entity2 = new Entity();
	entity2->AddComponent(new DrawnMesh(render, mesh2, basicMaterial));
	ents.push_back(entity2);
	meshes.push_back(mesh2);

	normal = XMFLOAT3(0, 0, -1);
	Vertex vertices3[] =
	{
		{ XMFLOAT3(-1.5f, +1.0f, +0.0f), normal, uv },// 0
		{ XMFLOAT3(+1.5f, +1.0f, 0.0f), normal, uv },// 1
		{ XMFLOAT3(+1.5f, -1.0f, +0.0f), normal, uv },// 2
	};
	UINT indices3[] = { 0, 1, 2 };
	Mesh* mesh3 = new Mesh("Assets/Models/sphere.obj", device);//vertices3, 3, indices3, 3
	Entity* entity3 = new Entity();
	entity3->AddComponent(new DrawnMesh(render, mesh3, basicMaterial));
	ents.push_back(entity3);
	meshes.push_back(mesh3);
}

#pragma endregion

#pragma region Window Resizing

// --------------------------------------------------------
// Handles resizing DirectX "stuff" to match the (usually) new
// window size and updating our projection matrix to match
// --------------------------------------------------------
void MyDemoGame::OnResize()
{
	// Handle base-level DX resize stuff
	DirectXGameCore::OnResize();

	// Update our projection matrix since the window size changed
	camera.CreatePerspectiveProjectionMatrix(aspectRatio, 0.1f, 100.0f);
}
#pragma endregion

#pragma region Game Loop

// --------------------------------------------------------
// Update your game here - take input, move objects, etc.
// --------------------------------------------------------
float x = 0;
void MyDemoGame::UpdateScene(float deltaTime, float totalTime)
{
	// Quit if the escape key is pressed
	if (GetAsyncKeyState(VK_ESCAPE))
		Quit();

	DirectX::XMFLOAT3 rot = ents[0]->GetTransform().GetRotation();
	float rotRate = 3.0f;
	rot.x += rotRate * deltaTime;
	rot.y += rotRate * deltaTime;
	rot.z += rotRate * deltaTime;
	render->GetLight(0).GetTransform().SetRotation(rot);
	ents[0]->GetTransform().SetRotation(rot);
	DirectX::XMFLOAT3 pos = ents[1]->GetTransform().GetPosition();
	pos.x += 0.2f * deltaTime;
	//entity2->GetTransform().SetPosition(pos);
	ents[2]->GetTransform().SetParrent(&ents[1]->GetTransform());

	for (int e = 0; e < ents.size(); e++) {
		ents[e]->Update();
	}

	//Temp camera and input stuff
	LONG deltaMouseX = curMousePos.x - prevMousePos.x;
	LONG deltaMouseY = curMousePos.y - prevMousePos.y;
	camera.Update(deltaTime, deltaMouseX, deltaMouseY);
	prevMousePos.x = curMousePos.x;
	prevMousePos.y = curMousePos.y;
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void MyDemoGame::DrawScene(float deltaTime, float totalTime)
{
	// Background color (Cornflower Blue in this case) for clearing
	const float color[4] = {0.4f, 0.6f, 0.75f, 0.0f};

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of DrawScene (before drawing *anything*)
	deviceContext->ClearRenderTargetView(renderTargetView, color);
	deviceContext->ClearDepthStencilView(
		depthStencilView, 
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	render->UpdateAndRender(camera);


	// Present the buffer
	//  - Puts the image we're drawing into the window so the user can see it
	//  - Do this exactly ONCE PER FRAME
	//  - Always at the very end of the frame
	HR(swapChain->Present(0, 0));
}

#pragma endregion

#pragma region Mouse Input

// --------------------------------------------------------
// Helper method for mouse clicking.  We get this information
// from the OS-level messages anyway, so these helpers have
// been created to provide basic mouse input if you want it.
//
// Feel free to add code to this method
// --------------------------------------------------------
void MyDemoGame::OnMouseDown(WPARAM btnState, int x, int y)
{
	if (prevMousePos.x == 0 && prevMousePos.y == 0) {
		prevMousePos.x = curMousePos.x;
		prevMousePos.y = curMousePos.y;
	}
	// Save the previous mouse position, so we have it for the future
	curMousePos.x = x;
	curMousePos.y = y;

	// Caputure the mouse so we keep getting mouse move
	// events even if the mouse leaves the window.  we'll be
	// releasing the capture once a mouse button is released
	SetCapture(hMainWnd);
}

// --------------------------------------------------------
// Helper method for mouse release
//
// Feel free to add code to this method
// --------------------------------------------------------
void MyDemoGame::OnMouseUp(WPARAM btnState, int x, int y)
{
	// We don't care about the tracking the cursor outside
	// the window anymore (we're not dragging if the mouse is up)
	ReleaseCapture();
}

// --------------------------------------------------------
// Helper method for mouse movement.  We only get this message
// if the mouse is currently over the window, or if we're 
// currently capturing the mouse.
//
// Feel free to add code to this method
// --------------------------------------------------------
void MyDemoGame::OnMouseMove(WPARAM btnState, int x, int y)
{
	if (prevMousePos.x == 0 && prevMousePos.y == 0) {
		prevMousePos.x = curMousePos.x;
		prevMousePos.y = curMousePos.y;
	}
	// Save the previous mouse position, so we have it for the future
	curMousePos.x = x;
	curMousePos.y = y;
}
#pragma endregion