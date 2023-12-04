#pragma once

#include <windows.h>

class CelestiaCore;

namespace celestia::win32
{

void HandleCaptureMovie(HINSTANCE appInstance, HWND hWnd, CelestiaCore* appCore);

}
