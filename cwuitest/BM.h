#pragma once
#include "cwsdk/cwsdk.h"

#define BUILDING_MOD_DLL "BuildingMod.dll"

namespace BM {
	// Building mod globals
	typedef void(__cdecl* BM_PrintSelectBlockMessage_t)(unsigned char r, unsigned char g, unsigned char b, unsigned char type);

	extern bool* currently_building;
	extern BlockColor* current_block_color;
	extern bool* building_underwater;
	extern BM_PrintSelectBlockMessage_t PrintSelectBlockMessage;

	void InitGlobals();
	void SetBlock(BlockColor bc);
};

