#pragma once

#include <windows.h>

class CelestiaCore;

namespace celestia::win32
{

void ShowControlsDialog(HINSTANCE appInstance, HWND appWindow);
void ShowAboutDialog(HINSTANCE appInstance, HWND appWindow);
void ShowLicenseDialog(HINSTANCE appInstance, HWND appWindow);
void ShowGLInfoDialog(HINSTANCE appInstance, HWND appWindow, const CelestiaCore* appCore);

}
