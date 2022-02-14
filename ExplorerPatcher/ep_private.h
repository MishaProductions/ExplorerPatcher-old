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

HBITMAP CreateBitmap(LONG width, LONG height)
{
	HBITMAP bitmap;
	BITMAPINFO bitmapInfo;
	ZeroMemory(&bitmapInfo, sizeof(BITMAPINFO));

	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	memset(&bitmapInfo.bmiHeader.biCompression, 0, 28);
	bitmapInfo.bmiHeader.biWidth = width;
	bitmapInfo.bmiHeader.biHeight = height;
	bitmapInfo.bmiHeader.biPlanes = 2097153;
	bitmapInfo.bmiHeader.biBitCount = 32;

	bitmap = CreateDIBSection(NULL, &bitmapInfo, 0, NULL, 0, 0);

	if (bitmap == NULL)
	{
		printf("Error while creating bitmap: %x\n", GetLastError());
	}
	return bitmap;
}

void DoDrawBitmap(WPARAM wparam, long explorerRectLeft, long explorerRecTop, long ExplorerRectWidth,
	long ExplorerRectHeight, DWORD unknown, byte unknown_2)

{
	HDC hdc = CreateCompatibleDC(NULL);
	HBITMAP Bitmap = CreateBitmap(1, 1);
	DWORD blendfunction_struct;

	if (Bitmap == NULL) {
		printf("bitmap is null!\n");
	}
	else {
		if (SelectObject(hdc, Bitmap) == NULL) {
			printf("SelectObject() failed\n");
		}
	}

	if (unknown_2 == 0xff) {
		if (!StretchBlt((HDC)wparam, explorerRectLeft, explorerRecTop, ExplorerRectWidth, ExplorerRectHeight,
			hdc, 0, 0, 1, 1, 0xcc0020))
		{
			printf("StretchBlt() failed\n");
		}
	}
	else {
		blendfunction_struct = (DWORD)unknown_2 << 0x10;

		BYTE* arrData = &blendfunction_struct;
		//TODO: very hacky
		BLENDFUNCTION func;
		func.BlendOp = arrData[0];
		func.BlendFlags = arrData[1];
		func.SourceConstantAlpha = arrData[2];
		func.AlphaFormat = arrData[3];

		if (!GdiAlphaBlend((HDC)wparam, explorerRectLeft, explorerRecTop, ExplorerRectWidth, ExplorerRectHeight
			, hdc, 0, 0, 1, 1, func))
		{
			printf("GdiAlphaBlend() failed\n");
		}
	}

	DeleteObject(Bitmap);
	DeleteDC(hdc);
}
void CallDrawBitmap(WPARAM wparam, RECT* explorerRectLeft, DWORD unknown, byte unknown_2)

{
	DoDrawBitmap(wparam, explorerRectLeft->left, explorerRectLeft->top,
		explorerRectLeft->right - explorerRectLeft->left,
		explorerRectLeft->bottom - explorerRectLeft->top, unknown, unknown_2);
}


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
LRESULT ReBarWindow32Hook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData)
{
	if (uMsg == 0x200b) {
		//fix for dark mode
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
	options.dwFlags = 3;// propVal == NULL ? 3 : 0; //propVal is null when it is control panel

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