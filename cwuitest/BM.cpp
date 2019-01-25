#include "BM.h"

#include <Windows.h>

namespace BM {
	// Building mod globals
	typedef void(__cdecl* BM_PrintSelectBlockMessage_t)(unsigned char r, unsigned char g, unsigned char b, unsigned char type);

	extern bool* currently_building = nullptr;
	extern BlockColor* current_block_color = nullptr;
	extern bool* building_underwater = nullptr;
	extern BM_PrintSelectBlockMessage_t PrintSelectBlockMessage = nullptr;

	void InitGlobals() {
		auto BMHnd = GetModuleHandleA(BUILDING_MOD_DLL);
		auto GetCurrentlyBuilding = (bool*(__cdecl*)())GetProcAddress(BMHnd, "GetCurrentlyBuilding");
		auto GetCurrentBlockColorType = (BlockColor*(__cdecl*)())GetProcAddress(BMHnd, "GetCurrentBlockColorType");
		auto GetBuildingUnderwater = (bool*(__cdecl*)())GetProcAddress(BMHnd, "GetBuildingUnderwater");

		currently_building = GetCurrentlyBuilding();
		current_block_color = GetCurrentBlockColorType();
		building_underwater = GetBuildingUnderwater();

		PrintSelectBlockMessage = (BM_PrintSelectBlockMessage_t)GetProcAddress(BMHnd, "PrintSelectBlockMessage");
	}


	void SetBlock(BlockColor bc) {
		*current_block_color = bc;
		PrintSelectBlockMessage(bc.Red, bc.Green, bc.Blue, bc.Type);
	}
};
