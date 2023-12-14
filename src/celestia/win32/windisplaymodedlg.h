#pragma once

#include <celutil/array_view.h>

#include <windows.h>

namespace celestia::win32
{

class DisplayModeDialog
{
public:
    DisplayModeDialog(HINSTANCE, HWND, util::array_view<DEVMODE>, int);

    HWND parent;
    HWND hwnd{ nullptr };
    util::array_view<DEVMODE> displayModes;
    int screenMode;
    bool update{ false };
};

}
