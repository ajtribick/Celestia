#pragma once

#include <windows.h>
#include <oleidl.h>

class CelestiaCore;

namespace celestia::win32
{

//
// A very minimal IDropTarget interface implementation
//
class CelestiaDropTarget : public IDropTarget
{
public:
    explicit CelestiaDropTarget(CelestiaCore*);
    ~CelestiaDropTarget() = default;

    STDMETHOD  (QueryInterface)(REFIID idd, void** ppvObject);
    STDMETHOD_ (ULONG, AddRef) (void);
    STDMETHOD_ (ULONG, Release) (void);

    // IDropTarget methods
    STDMETHOD (DragEnter)(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD (DragOver) (DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD (DragLeave)(void);
    STDMETHOD (Drop)     (LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);

private:
    CelestiaCore* appCore;
    ULONG refCount{ 0 };
};

} // end namespace celestia::win32
