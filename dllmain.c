#include <Windows.h>
#include <stdio.h>
#include "Minhook/include/MinHook.h"

// #define log(message, ...) printf("[Info] " message "\n", ##__VA_ARGS__)
#define log(message, ...) 


__attribute__((section ("SHARED"), shared)) int state = 0;
__attribute__((section ("SHARED"), shared)) wchar_t targetName[MAX_PATH] = { '\0' };
__attribute__((section ("SHARED"), shared)) int clickPosX, clickPosY;
__attribute__((section ("SHARED"), shared)) int triggerKey;

typedef WINBOOL(WINAPI *GetCursorPosT)(LPPOINT lpPoint);
typedef WINBOOL(WINAPI *SetCursorPosT)(int x, int y);
typedef WINBOOL(WINAPI *ClipCursorT)(const RECT *lpRect);

HWND targetHwnd = NULL;

WNDPROC origin_wndProc = NULL;
GetCursorPosT origin_getCursorPos = NULL;
SetCursorPosT origin_setCursorPos = NULL;
SetCursorPosT origin_setPhysicalCursorPos = NULL;
ClipCursorT origin_clipCursor = NULL;

BOOLEAN is_activate = FALSE;
BOOLEAN is_autoClick = FALSE;
BOOLEAN is_exit = FALSE;


BOOLEAN createConsole() {
	AllocConsole();
	FILE* stream;
	freopen_s(&stream, "CON", "r", stdin);
	freopen_s(&stream, "CON", "w", stdout);

	return TRUE;
}



WINBOOL WINAPI hooked_GetCursorPos(LPPOINT lpPoint){
	if(is_autoClick){
		*lpPoint = (POINT){clickPosX, clickPosY};
		return TRUE;
	}
	else 
		return origin_getCursorPos(lpPoint);
}

WINBOOL WINAPI hooked_SetCursorPos(int x, int y) {
	log("SetCursorPos %d %d\n", x, y);
	if(is_activate)
		return origin_setCursorPos(x, y);
	else 
		return TRUE;
}

WINBOOL WINAPI hooked_SetPhysicalCursorPos(int x, int y) {
	log("SetPhysicalCursorPos %d %d\n", x, y);
	if(is_activate)
		return origin_setPhysicalCursorPos(x, y);
	else 
		return TRUE;
}

WINBOOL WINAPI hooked_ClipCursor(const RECT *lpRect) {
	log("ClipCursor\n");
	if(is_activate)
		return origin_clipCursor(lpRect);
	else
		return TRUE;
}



LRESULT __stdcall WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_KEYDOWN:
		if(wParam == triggerKey)
			return DefWindowProc(hwnd, msg, wParam, lParam);
		break;
		
	case WM_KEYUP:
		if(wParam == triggerKey) {
			is_autoClick = !is_autoClick;
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		break;

	// case WM_LBUTTONUP:
	// 	POINT pt;
	// 	origin_getCursorPos(&pt);
	// 	log("Mouse Click: (%d, %d)\n", pt.x, pt.y);
	// 	break;

	case WM_ACTIVATE:
		is_activate = wParam;
		if(is_autoClick && lParam == FALSE)
			return DefWindowProc(hwnd, msg, wParam, lParam);
		break;

	default: break;
	}

	return CallWindowProcW(origin_wndProc, hwnd, msg, wParam, lParam);
}



BOOL CALLBACK enumWindowsProc(HWND hWnd, LPARAM lParam) {
    DWORD pid = GetCurrentProcessId();
    DWORD hWnd_pid = 0;
    GetWindowThreadProcessId(hWnd, &hWnd_pid);
    if (pid == hWnd_pid) {
        *(HWND*)lParam = hWnd;
        return FALSE;
    }
    return TRUE;
}



HWND getCurrentHWND() {
    HWND hwnd = NULL;
    EnumWindows(enumWindowsProc, (LPARAM)&hwnd);
    return hwnd;
}



BOOLEAN is_hooked = FALSE;

DWORD hook(void* lpThreadParameter) {
	is_hooked = TRUE;
	while(targetHwnd == NULL) {
		targetHwnd = getCurrentHWND();
		Sleep(100);
	}

	if(!targetHwnd) {
		MessageBoxW(NULL, L"Faild to get HWND!\n", L"ERROR", MB_OK);
		return 0;
	}

	// createConsole();

	MH_Initialize();
	MH_CreateHook((LPVOID)GetCursorPos, (LPVOID)hooked_GetCursorPos, (LPVOID*)&origin_getCursorPos);
	MH_CreateHook((LPVOID)SetPhysicalCursorPos, (LPVOID)hooked_SetPhysicalCursorPos, (LPVOID*)&origin_setPhysicalCursorPos);
	MH_CreateHook((LPVOID)SetCursorPos, (LPVOID)hooked_SetCursorPos, (LPVOID*)&origin_setCursorPos);
	MH_CreateHook((LPVOID)ClipCursor, (LPVOID)hooked_ClipCursor, (LPVOID*)&origin_clipCursor);
	MH_EnableHook(MH_ALL_HOOKS);

	origin_wndProc = (WNDPROC)SetWindowLongPtrW(targetHwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
	if(!origin_wndProc) {
		MessageBoxW(NULL, L"Faild to set WndProc!\n", L"ERROR", MB_OK);
		return 0;
	}

	while(TRUE) {
		if(is_autoClick){
			PostMessageW(targetHwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(clickPosX, clickPosY));
			Sleep(75);
			PostMessageW(targetHwnd, WM_LBUTTONUP, 1, MAKELPARAM(clickPosX, clickPosY));
		}

		if(is_exit)
			return 0;

		Sleep(200);
	}

	return 0;
}



void unhook() {
	is_exit = TRUE;
	Sleep(600);

	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
	// FreeConsole();

	if(origin_wndProc)
		SetWindowLongPtrW(targetHwnd, GWLP_WNDPROC, (LONG_PTR)origin_wndProc);
}



int varifyProcess() {
	if(state == 0) {
		state = 1;
		return 0;
	}
		
	wchar_t processName[MAX_PATH], *processBaseName;
	if (!GetModuleFileNameW(NULL, processName, MAX_PATH))
		return -1;

	processBaseName = wcsrchr(processName, L'\\');

	if (!processBaseName)
		return -1;

	processBaseName++;

	if (_wcsicmp(processBaseName, targetName) == 0)
		return 1;

	return -1;
}



BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		int rc = varifyProcess();
		if (rc < 0)  return FALSE;
		if (rc == 0) return TRUE;
		CreateThread(NULL, 0, hook, NULL, 0, NULL);
	}

	else if (ul_reason_for_call == DLL_PROCESS_DETACH && is_hooked)
		unhook();
		
    return TRUE;
}



#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) LRESULT CALLBACK hookProc(int code, WPARAM wParam, LPARAM lParam) {
	return CallNextHookEx(NULL, code, wParam, lParam);
}

__declspec(dllexport) WINAPI void setTargetName(wchar_t _targetName[]) {
	wcscpy_s(targetName, MAX_PATH, _targetName);
}

__declspec(dllexport) WINAPI void setClickPos(int x, int y) {
	clickPosX = x;
	clickPosY = y;
}

__declspec(dllexport) WINAPI void setTrigerKey(int keycode) {
	triggerKey = keycode;
}

#ifdef __cplusplus
}
#endif
