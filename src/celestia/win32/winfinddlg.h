#pragma once

#include <windows.h>

class CelestiaCore;

namespace celestia::win32
{

void ShowFindObjectDialog(HINSTANCE appInstance,
                          HWND appWindow,
                          const CelestiaCore* celestiaCore);

}
