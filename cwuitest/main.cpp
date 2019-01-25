#include "cwsdk/cwsdk.h"
#include "BM.h"
#include "Util.h"


#include <windows.h>
#include <iostream>
#include <fstream>

#include <string>
#include <cstdint>
#include <sstream>
#include <vector>
#include <bitset>
#include <map>
#include <d3d9.h>

#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_dx9.h"
#include "imgui/examples/imgui_impl_win32.h"


typedef HRESULT(__stdcall* EndScene_t)(LPDIRECT3DDEVICE9 pDevice);
typedef HRESULT(__stdcall* Reset_t)(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
EndScene_t OriginalEndScene = nullptr;
Reset_t OriginalReset = nullptr;
WNDPROC OriginalWndProc = nullptr;

bool imgui_inited = false;
bool imgui_wants_mouse = false;


namespace prev_frame {
	bool colored_block = false;
	bool water_block = false;
	bool solid_wet_block = false;
	bool build_underwater = false;
};

bool colored_block = false;
bool water_block = false;
bool solid_wet_block = false;
bool build_underwater = false;

ImVec4 block_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

HRESULT __stdcall EndSceneHook(LPDIRECT3DDEVICE9 pDevice) {

	// Imgui uses TLS and needs to be initalized from the graphics thread.
	if (!imgui_inited) {

		imgui_inited = true;
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGui_ImplWin32_Init(GetActiveWindow());
		ImGui_ImplDX9_Init(pDevice);

		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontFromFileTTF("resource1.dat", 16.0f);
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	imgui_wants_mouse = ImGui::GetIO().WantCaptureMouse;
	
	if(*BM::currently_building){
		ImGui::Begin("Building Mod");

		ImGui::Checkbox("Colored Block", &colored_block);
		if (colored_block) {
			// Update UI block color from BM's internal block color
			/*
			auto upscale = [](float f) {
				return byte(max(0, min(255, (int)floor(f * 256.0))));
			};
			*/
			auto downscale = [](byte b) {
				return float(b / 255.0);
			};
			block_color.x = downscale(BM::current_block_color->Red);
			block_color.y = downscale(BM::current_block_color->Green);
			block_color.z = downscale(BM::current_block_color->Blue);

			ImGui::Text("Select your block color:");
			if (ImGui::ColorEdit3("Block Color", (float*)&block_color)) {
				//std::cout << "ColorEdit returned true" << std::endl;
				BM::SetBlock(BlockColor((int)(block_color.x / (1 / 255.0f)), (int)(block_color.y / (1 / 255.0f)), (int)(block_color.z / (1 / 255.0f)), 1));
			}
			if (colored_block != prev_frame::colored_block) {
				water_block = false;
				solid_wet_block = false;
			}
		}

		ImGui::Checkbox("Water", &water_block);
		if (water_block) {
			if (water_block != prev_frame::water_block) {
				BM::SetBlock(BlockColor(0, 0, 0, 2));
				colored_block = false;
				solid_wet_block = false;
			}
		}
		

		ImGui::Checkbox("Solid Wet Block", &solid_wet_block);
		if (solid_wet_block) {
			if (solid_wet_block != prev_frame::solid_wet_block) {
				BM::SetBlock(BlockColor(BM::current_block_color->Red, BM::current_block_color->Green, BM::current_block_color->Blue, 3));
				colored_block = false;
				water_block = false;
			}
		}
		

		ImGui::Separator();

		ImGui::Checkbox("Build underwater", &build_underwater);
		if (build_underwater != prev_frame::build_underwater) {
			*BM::building_underwater = build_underwater;
			std::cout << "BM_building_underwater now:" << *BM::building_underwater << std::endl;
		}

		prev_frame::colored_block = colored_block;
		prev_frame::water_block = water_block;
		prev_frame::solid_wet_block = solid_wet_block;
		prev_frame::build_underwater = build_underwater;



		ImGui::End();
	}

	ImGui::EndFrame();	
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());


	return OriginalEndScene(pDevice);
}



HRESULT __stdcall ResetHook(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
	if (imgui_inited) {
		ImGui_ImplDX9_InvalidateDeviceObjects();
		HRESULT result = OriginalReset(pDevice, pPresentationParameters);
		ImGui_ImplDX9_CreateDeviceObjects();
		return result;
	}

	return OriginalReset(pDevice, pPresentationParameters);
}


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CURSORINFO ci;
	ci.cbSize = sizeof(CURSORINFO);
	GetCursorInfo(&ci);
	auto cursor_showing = ci.flags & 0x1;

	if (imgui_wants_mouse) {
		if (!cursor_showing) {
			ShowCursor(true);
		}

		*cube::input_enabled = false;
		auto result = ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

		if (result)
			return result;
	}
	else {
		if (cursor_showing) {
			ShowCursor(false);
		}
		*cube::input_enabled = true;
	}

	return CallWindowProc(OriginalWndProc, hWnd, msg, wParam, lParam);
}



DWORD WINAPI MyFunc(LPVOID lpvParam) {
	//OpenConsole();

	// Busywait for building mod to load before doing anything else.
	// This is for future compatibillity if chris ever changes how the mod loader works.
	std::cout << "Waiting for BuildingMod.dll to load" << std::endl;
	while (!GetModuleHandle(BUILDING_MOD_DLL)) {
		Sleep(500);
	}
	std::cout << "BuildingMod.dll is loaded, now starting ui mod." << std::endl;

	cube::InitGlobals();
	BM::InitGlobals();
	auto gc = cube::BusywaitForGameController();

	try
	{
		
		auto vtable = *(IDirect3DDevice9_VTable**)gc->SomeDirect3DDevice9;

		// Hook EndScene
		OriginalEndScene = (EndScene_t)vtable->EndScene;
		vtable->EndScene = (void*)EndSceneHook;

		// Hook Reset
		OriginalReset = (Reset_t)vtable->Reset;
		vtable->Reset = (void*)ResetHook;

		std::cout << "Active window:" << std::hex << GetActiveWindow() << std::endl;
		OriginalWndProc = (WNDPROC)SetWindowLongPtr(*cube::hwnd, GWL_WNDPROC, (LONG)(LONG_PTR)WndProcHook);

		std::cout << "EndScene After:" << std::hex << vtable->EndScene << std::endl;
	}
	catch (const std::exception&)
	{
		std::cout << "got exception" << std::endl;
	}

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(NULL, 0, MyFunc, 0, 0, NULL);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

