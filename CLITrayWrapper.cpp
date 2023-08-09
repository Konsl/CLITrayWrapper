// CLITrayWrapper.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "CLITrayWrapper.h"
#include <iostream>
#include <conio.h>
#include <vector>

#define TRAY_ICON_UID		1

#define WM_TRAY_ICON		WM_APP
#define WM_EXITED			WM_TRAY_ICON + 1

HINSTANCE hInst;
HWND hWnd;
HWND hConsoleWnd;
HICON hIcon;

bool isVisible = true;

std::wstring title;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UpdateTrayIcon(HWND hWnd);
void DestroyTrayIcon(HWND hWnd);
void ToggleWindowVisibility();
void SetConsoleIcon();
void InitConsoleStreams();
std::vector<std::wstring> CommandLineToArgvVector(LPCWSTR lpCmdLine);
std::wstring ArgvToCommandLine(std::vector<std::wstring> argv);
std::wstring GetLastErrorMessage();

VOID NTAPI OnExited(PVOID ctx, BOOLEAN timeout) {
	SendMessage(hWnd, WM_EXITED, 0, 0);
}

void Pause() {
	std::wcout << L"Press any key to continue . . .";
	static_cast<void>(_getch());
}

void PrintHelp() {
	std::wcout << L"Usage: cli-tray-wrapper [initial state (visible|hidden)] [tray icon tooltip] [command...]" << std::endl;
	Pause();
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	hInst = hInstance;
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	if (!AllocConsole()) return 1;
	hConsoleWnd = GetConsoleWindow();
	if (hConsoleWnd == INVALID_HANDLE_VALUE) return 1;

	std::vector<std::wstring> args = CommandLineToArgvVector(lpCmdLine);
	if (args.size() < 3) {
		InitConsoleStreams();
		PrintHelp();
		FreeConsole();
		return 0;
	}

	boolean initiallyVisible = args[0] == L"visible";
	if (!initiallyVisible && args[0] != L"hidden") {
		InitConsoleStreams();
		std::wcout << L"Invalid value \"" << args[0] << L"\" for initial state" << std::endl;
		PrintHelp();
		FreeConsole();
		return 0;
	}

	if (!initiallyVisible) ToggleWindowVisibility();

	title = args[1];

	args.erase(args.begin(), args.begin() + 2);
	std::wstring commandLine = ArgvToCommandLine(args);

	WNDCLASS wndClass = {};
	wndClass.lpfnWndProc = WndProc;
	wndClass.hInstance = hInstance;
	wndClass.lpszClassName = L"MainWindow";
	RegisterClass(&wndClass);

	hWnd = CreateWindowEx(WS_EX_LEFT, L"MainWindow", L"CLITrayWrapper",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, (HWND)NULL,
		(HMENU)NULL, hInstance, (LPVOID)NULL);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcess(NULL, &commandLine[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		std::wstring msg = GetLastErrorMessage();
		DestroyTrayIcon(hWnd);
		InitConsoleStreams();
		std::wcout << L"Error: " << msg;
		Pause();

		FreeConsole();
		return 0;
	}

	HANDLE hWaitExit;
	RegisterWaitForSingleObject(&hWaitExit, pi.hProcess, OnExited, NULL, INFINITE, WT_EXECUTEONLYONCE);

	std::wstring executablePath;
	executablePath.resize(MAX_PATH);
	GetModuleFileNameEx(pi.hProcess, NULL, &executablePath[0], (DWORD)executablePath.size());

	if (ExtractIconEx(executablePath.c_str(), 0, &hIcon, NULL, 1) != 1)
		hIcon = LoadIcon(NULL, IDI_APPLICATION);
	if (hIcon == INVALID_HANDLE_VALUE) return 1;

	SetConsoleIcon();
	UpdateTrayIcon(hWnd);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DestroyIcon(hIcon);

	DWORD exitCode;
	GetExitCodeProcess(pi.hProcess, &exitCode);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	FreeConsole();
	return (int)msg.wParam;
}

void CreateTrayIcon(HWND hWnd) {
	NOTIFYICONDATA nid = {};
	nid.hWnd = hWnd;
	nid.uID = TRAY_ICON_UID;
	nid.uFlags = NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_TRAY_ICON;
	wcscpy_s(nid.szTip, title.c_str());

	Shell_NotifyIcon(NIM_ADD, &nid);
}

void UpdateTrayIcon(HWND hWnd) {
	NOTIFYICONDATA nid = {};
	nid.hWnd = hWnd;
	nid.uID = TRAY_ICON_UID;
	nid.uFlags = NIF_ICON;
	nid.hIcon = hIcon;

	Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void DestroyTrayIcon(HWND hWnd) {
	NOTIFYICONDATA nid = {};
	nid.hWnd = hWnd;
	nid.uID = TRAY_ICON_UID;

	Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ToggleWindowVisibility() {
	isVisible = !isVisible;
	ShowWindow(hConsoleWnd, isVisible ? SW_SHOW : SW_HIDE);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:
		CreateTrayIcon(hWnd);
		return 0;

	case WM_TRAY_ICON:
		switch (LOWORD(lParam)) {
		case WM_LBUTTONUP:
			ToggleWindowVisibility();
			break;
		}
		return 0;

	case WM_EXITED: {
		DestroyWindow(hWnd);
		return 0;
	}

	case WM_DESTROY:
		DestroyTrayIcon(hWnd);
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void SetConsoleIcon() {
	SendMessage(hConsoleWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	SendMessage(hConsoleWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
}

void InitConsoleStreams()
{
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	std::cout.clear();
	std::clog.clear();
	std::cerr.clear();
	std::cin.clear();

	HANDLE hConOut = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hConIn = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
	SetStdHandle(STD_ERROR_HANDLE, hConOut);
	SetStdHandle(STD_INPUT_HANDLE, hConIn);
	std::wcout.clear();
	std::wclog.clear();
	std::wcerr.clear();
	std::wcin.clear();
}

std::vector<std::wstring> CommandLineToArgvVector(LPCWSTR lpCmdLine) {
	int numArgs = 0;
	LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &numArgs);

	std::vector<std::wstring> output;
	for (int i = 0; i < numArgs; i++) {
		output.push_back(argv[i]);
	}

	LocalFree(argv);

	return output;
}

std::wstring ArgvToCommandLine(std::vector<std::wstring> argv) {
	std::wstring output;

	for (std::wstring arg : argv) {
		if (output.length() != 0) output.append(L" ");

		if (arg.find(L' ') == std::string::npos &&
			arg.find(L'\t') == std::string::npos &&
			arg.find(L'\n') == std::string::npos &&
			arg.find(L'\v') == std::string::npos &&
			arg.find(L'\"') == std::string::npos) output.append(arg);
		else {
			output.append(L"\"");

			for (size_t i = 0; i < arg.length(); i++) {
				size_t backslashes = 0;

				while (i < arg.length() && arg[i] == L'\\') {
					i++;
					backslashes++;
				}

				std::wstring backslashesString = arg.substr(i - backslashes, backslashes);

				if (i == arg.length())
					output.append(backslashesString).append(backslashesString);
				else if (arg[i] == L'\"') {
					i++;
					output.append(backslashesString).append(backslashesString);
				}
				else {
					output.append(backslashesString);
					output.push_back(arg[i]);
				}
			}

			output.append(L"\"");
		}
	}

	return output;
}

std::wstring GetLastErrorMessage() {
	DWORD error = GetLastError();

	LPWSTR buffer = NULL;
	size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buffer, 0, NULL);

	std::wstring msg(buffer, size);
	LocalFree(buffer);

	return msg;
}
