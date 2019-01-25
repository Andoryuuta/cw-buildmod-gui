#include "Util.h"

#include <Windows.h>
#include <cstdio>

void OpenConsole()
{
	AllocConsole();
	FILE* cinStream;
	FILE* coutStream;
	FILE* cerrStream;
	freopen_s(&cinStream, "CONIN$", "r", stdin);
	freopen_s(&coutStream, "CONOUT$", "w", stdout);
	freopen_s(&cerrStream, "CONOUT$", "w", stderr);
}