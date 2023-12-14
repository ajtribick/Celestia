#pragma once

#include <windows.h>

class CelestiaCore;

namespace celestia::win32
{

void HandleCaptureImage(HWND hWnd, CelestiaCore* appCore);
void HandleOpenScript(HWND hWnd, CelestiaCore* appCore);

}
