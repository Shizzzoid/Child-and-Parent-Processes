#include "framework.h"
#include "Child.Process.h"
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <windows.h>
#include <shellapi.h>
#include <cstdlib>
#include <ctime>
#include <vector>

#pragma comment(lib, "shell32.lib")
#define MAX_LOADSTRING 100
#define WM_UPDATE_TIME (WM_USER + 1)
#define WM_CHILD_COMPLETED (WM_USER + 2)

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
HWND hWndParent;
HWND hWnd;

const int windowWidth = 350;
const int windowHeight = 100;

std::vector<RECT> windowPositions;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, int, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int processNumber;
int lifeTime;

void DrawProcessInfo(HWND hWnd, int processNumber, int lifeTime)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    std::wstring infoText = L"Процесс #" + std::to_wstring(processNumber) + L" \nОсталось времени: " + std::to_wstring(lifeTime) + L" секунд";
    TextOut(hdc, 10, 10, infoText.c_str(), infoText.length());

    EndPaint(hWnd, &ps);
}

DWORD WINAPI TimerThread(LPVOID lpParam)
{
    HWND hWnd = (HWND)lpParam;

    while (lifeTime > 0)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        --lifeTime;

        InvalidateRect(hWnd, NULL, TRUE);
        UpdateWindow(hWnd);

        SendMessage(hWndParent, WM_UPDATE_TIME, (WPARAM)processNumber, (LPARAM)lifeTime);
    }

    SendMessage(hWndParent, WM_CHILD_COMPLETED, (WPARAM)processNumber, 0);

    SendMessage(hWnd, WM_CLOSE, 0, 0);
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    int argc;
    LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);
    if (argc != 5)
    {
        std::wcerr << L"Неверное количество аргументов\n";
        return 1;
    }

    processNumber = _wtoi(argv[0]);
    lifeTime = _wtoi(argv[1]);
    hWndParent = (HWND)_wtol(argv[2]);
    int posX = _wtoi(argv[3]);
    int posY = _wtoi(argv[4]);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CHILDPROCESS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow, posX, posY))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CHILDPROCESS));

    CreateThread(NULL, 0, TimerThread, (LPVOID)hWnd, 0, NULL);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CHILDPROCESS));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_CHILDPROCESS);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, int posX, int posY)
{
    hInst = hInstance;

    RECT desktop;
    GetClientRect(GetDesktopWindow(), &desktop);
    int screenWidth = desktop.right;
    int screenHeight = desktop.bottom;

    const int windowWidth = 350;
    const int windowHeight = 100;

    if (posX + windowWidth > screenWidth) 
    {
        posX = screenWidth - windowWidth;
    }
    if (posY + windowHeight > screenHeight) 
    {
        posY = screenHeight - windowHeight;
    }

    hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, posX, posY, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
        DrawProcessInfo(hWnd, processNumber, lifeTime);
        break;
    case WM_UPDATE_TIME:
        InvalidateRect(hWnd, NULL, TRUE);
        UpdateWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
