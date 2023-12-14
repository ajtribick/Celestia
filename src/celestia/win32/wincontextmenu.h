#pragma once

#include <celestia/celestiacore.h>

#include <windows.h>

class Selection;

namespace celestia::win32
{

class MainWindow;

constexpr inline UINT MENU_CHOOSE_PLANET  = 32000;
constexpr inline UINT MENU_CHOOSE_SURFACE = 31000;

class WinContextMenuHandler : public CelestiaCore::ContextMenuHandler
{
public:
    WinContextMenuHandler(const CelestiaCore*, HWND, MainWindow*);

    void requestContextMenu(float x, float y, Selection sel) override;
    void setHwnd(HWND newHWnd) { hwnd = newHWnd; }

private:
    const CelestiaCore* appCore;
    HWND hwnd;
    MainWindow* mainWindow;
};

}
