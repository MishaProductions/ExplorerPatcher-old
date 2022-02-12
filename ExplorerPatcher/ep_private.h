#include <wtypes.h>

#pragma region  Tray icon skinning
HANDLE SystemTray_LoadImageWHook(
    HINSTANCE hInst,
    LPCWSTR   name,
    UINT      type,
    int       cx,
    int       cy,
    UINT      fuLoad
) {
    return LoadImageW(hInst, name, type, cx, cy, fuLoad);
}
#pragma endregion
#pragma region Explorer / Control panel mica
bool CheckIfWindowIsFloating(HWND explorerHandle)
{
    HWND hWnd = GetParent(explorerHandle);

    //alloc our buffer
    WCHAR buffer[100];
    buffer[0] = L'\0';

    GetWindowTextW(hWnd, buffer, 100);

    //Check if the parent window is a floating window
    return lstrcmpiW(buffer, L"FloatingWindow") == 0;
}
//This method actually applies the mica affect
void ApplyDWMMica(HWND explorerHandle)
{
    HWND hwnd = GetAncestor(explorerHandle, 2);
    WTA_OPTIONS options;
    MARGINS margins;

    //Get explorer rect
    RECT explorerTopRect;
    GetWindowRect(explorerHandle, &explorerTopRect);

    MapWindowPoints(NULL, hwnd, (LPPOINT)&explorerTopRect, 2);

    //Create margins for DwmExtendFrameIntoClientArea
    margins.cxLeftWidth = 0;
    margins.cxRightWidth = 0;
    margins.cyBottomHeight = 0;
    margins.cyTopHeight = explorerTopRect.bottom;

    //We don't want to do anything if the window is floating
    if (CheckIfWindowIsFloating(explorerHandle)) {
        margins.cyTopHeight = 0;
    }
    //hwnd = GetAncestor(explorerHandle, 2);

    HRESULT hr = DwmExtendFrameIntoClientArea(hwnd, &margins);
    if (hr != 0) {
        printf("DwmExtendFrameIntoClientArea() failed\n");
    }

    HANDLE propVal = GetPropW(hwnd, (LPCWSTR)0xa91c);
    options.dwMask = WTNCA_NODRAWICON | WTNCA_NODRAWCAPTION;
    options.dwFlags = propVal == NULL ? 3 : 0; //propVal is null when it is control panel

    //Disable caption and icon on window title bar
    hr = SetWindowThemeAttribute(hwnd, WTA_NONCLIENT, &options, 8);
    if (hr != 0) {
        printf("SetWindowThemeAttribute() failed\n");
    }
}
LRESULT ReBarWindow32Hook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData)
{
    if (uMsg == 0x200b) {
        //fix for dark mode?
        if (wcsncmp((wchar_t*)lParam, L"DarkMode", 8) == 0) {
            lParam = lParam + 0x10;
        }
    }
    else if (uMsg == WM_DESTROY) {
        //Clean up
        RemoveWindowSubclass(hWnd, ReBarWindow32Hook, (UINT_PTR)ReBarWindow32Hook);
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
LRESULT ExplorerMicaTitlebarSubclassProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData
)
{
    if (uMsg == WM_ERASEBKGND) {
        if (!CheckIfWindowIsFloating(hWnd)) {
            //Prevent Explorer from drawing the default background
            return 1;
        }
        else {
            printf("!!!!CheckIfWindowIsFloating()=true!!!!!\n");
        }
    }
    else {
        if (uMsg == WM_WINDOWPOSCHANGED) {
            LRESULT ret = DefSubclassProc(hWnd, WM_WINDOWPOSCHANGED, (WPARAM)wParam, (LPARAM)lParam);
            byte* ptr = ((byte*)lParam);
            if ((ptr[8] & 2) != 0) {
                ApplyDWMMica(hWnd);
                return ret;
            }
            PostMessageW(hWnd, 0x8000, 0, 0);
            return ret;
        }
        if (uMsg == 0x8000) {
            ApplyDWMMica(hWnd);
        }
        else if (uMsg == WM_PARENTNOTIFY) {
            if ((short)wParam == 1) {
                WORD classWord = GetClassWord((HWND)lParam, -0x20);
                UINT rebarwindow32 = RegisterWindowMessageW(L"ReBarWindow32");

                if (classWord == rebarwindow32) {
                    SetWindowSubclass((HWND)lParam, ReBarWindow32Hook, (UINT_PTR)ReBarWindow32Hook, 0);
                }
            }
        }
        else if (uMsg == WM_DESTROY) {
            //Remove the hook
            RemoveWindowSubclass(hWnd, ExplorerMicaTitlebarSubclassProc, (UINT_PTR)ExplorerMicaTitlebarSubclassProc);
        }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
#pragma endregion