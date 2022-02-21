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
	char buffer[100];
	GetModuleBaseNameA(GetCurrentProcess(), hInst, buffer, 100);
	printf("LoadImageW(): %d in %s, type: %d\n", name, buffer, type);

	return LoadImageW(hInst, name, type, cx, cy, fuLoad);
}
#pragma endregion
#pragma region Explorer / Control panel mica
LRESULT ReBarWindow32Hook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData)
{
	if (uMsg == 0x200b) {
		//fix for dark mode
		//Normally DarkModeInactiveNavbarComposited
		if (wcsncmp((wchar_t*)lParam, L"DarkMode", 8) == 0) {
			//Set it to InactiveNavbarComposited
			lParam = lParam + (sizeof(wchar_t) * 8);
		}
	}
	else if (uMsg == WM_DESTROY) {
		//Clean up
		RemoveWindowSubclass(hWnd, ReBarWindow32Hook, (UINT_PTR)ReBarWindow32Hook);
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void ApplyMicaToExplorerTitlebar(HWND navBar)
{
	HWND root = GetAncestor(navBar, GA_ROOT); //handle to entire explorer window

	//Get navbar rect
	RECT NavBarRect;
	GetWindowRect(navBar, &NavBarRect);

	MapWindowPoints(NULL, root, (LPPOINT)&NavBarRect, 2);

	//Extend window frame to the navigation bar
	MARGINS margins;
	margins.cxLeftWidth = 0;
	margins.cxRightWidth = 0;
	margins.cyBottomHeight = 0;
	margins.cyTopHeight = NavBarRect.bottom;

	HRESULT hr = DwmExtendFrameIntoClientArea(root, &margins);
	if (hr != 0) {
		printf("DwmExtendFrameIntoClientArea() failed\n");
	}

	WTA_OPTIONS options;

	options.dwMask = WTNCA_NODRAWICON | WTNCA_NODRAWCAPTION;
	options.dwFlags = GetPropW(root, (LPCWSTR)0xa91c) == NULL ? 3 : 0;

	hr = SetWindowThemeAttribute(root, WTA_NONCLIENT, &options, 8);
	if (hr != 0) {
		printf("SetWindowThemeAttribute() failed\n");
	}
}
//navigation bar hook
LRESULT ExplorerMicaTitlebarSubclassProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
)
{
	if (uMsg == WM_DESTROY)
	{
		//Clean up
		RemoveWindowSubclass(hWnd, ExplorerMicaTitlebarSubclassProc, (UINT_PTR)ExplorerMicaTitlebarSubclassProc);
	}
	if (uMsg == WM_ERASEBKGND)
	{
		//make the navigation bar background black
		return TRUE;
	}
	else if (uMsg == WM_WINDOWPOSCHANGED) {
		WINDOWPOS* p = (WINDOWPOS*)lParam;
		if ((p->flags & SWP_NOMOVE) != 0) {
			ApplyMicaToExplorerTitlebar(hWnd);
		}
		else {
			PostMessageW(hWnd, 0x8000, 0, 0);
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}
	else if (uMsg == WM_APP) {
		ApplyMicaToExplorerTitlebar(hWnd);
	}
	else if (uMsg == WM_PARENTNOTIFY) {
		//This is needed to enable mica in dark mode
		if ((short)wParam == 1) {
			WORD classWord = GetClassWord((HWND)lParam, -0x20);
			UINT rebarwindow32 = RegisterWindowMessageW(L"ReBarWindow32");

			if (classWord == rebarwindow32) {
				SetWindowSubclass((HWND)lParam, ReBarWindow32Hook, (UINT_PTR)ReBarWindow32Hook, 0);
			}
		}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
#pragma endregion