#include "framework.h"
#include "Parent.Process.h"
#include <CommCtrl.h>
#include <string>
#include <thread>
#include <vector>
#include <random>
#include <iostream>
#include <Windows.h>
#include <chrono>

#define MAX_LOADSTRING 100
#define IDC_INPUTTEXT 101
#define IDC_ERRORTEXT 102
#define IDC_ENTERBUTTON 103
#define IDC_TERMINATEBUTTON 105
#define WM_UPDATE_TIME (WM_USER + 1)
#define WM_CHILD_COMPLETED (WM_USER + 2)

HINSTANCE hInst;                                
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
std::vector<std::pair<int, int>> childProcesses;
std::vector<HWND> childWindows;
std::vector<HANDLE> processHandles;
int processCount = 0;
int targetProcessCount = 0;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void SetErrorText(HWND hWnd, LPCWSTR text, COLORREF color)
{
    HWND hErrorText = GetDlgItem(hWnd, IDC_ERRORTEXT);
    SetWindowText(hErrorText, text);

    HDC hdc = GetDC(hErrorText);
    SetTextColor(hdc, color);
    ReleaseDC(hErrorText, hdc);
}

HWND StartChildProcess(int processNumber, int lifeTime, int posX, int posY, HWND hWndParent, HANDLE& processHandle)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::wstring commandLine = L"F:\\ОС2.0\\Parent.Process\\Child.Process\\x64\\Debug\\Child.Process.exe " + std::to_wstring(processNumber) + L" " + std::to_wstring(lifeTime) + L" " + std::to_wstring((uintptr_t)hWndParent) + L" " + std::to_wstring(posX) + L" " + std::to_wstring(posY);

    if (!CreateProcess(NULL, const_cast<LPWSTR>(commandLine.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        std::wcerr << L"CreateProcess failed (" << GetLastError() << L").\n";
        return NULL;
    }

    CloseHandle(pi.hThread);

    processHandle = pi.hProcess;

    HWND hWndChild = NULL;
    DWORD processId = pi.dwProcessId;

    while (hWndChild == NULL)
    {
        EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
            {
                DWORD processId;
                GetWindowThreadProcessId(hWnd, &processId);
                if (processId == *reinterpret_cast<DWORD*>(lParam))
                {
                    *reinterpret_cast<HWND*>(lParam) = hWnd;
                    return FALSE;
                }
                return TRUE;
            }, reinterpret_cast<LPARAM>(&processId));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return hWndChild;
}

void StartNewProcess(HWND hWnd, int processNumber, int lifeTime)
{
    std::thread([=]() 
        {
        std::random_device rd;
        std::mt19937 gen(rd());
        RECT desktop;
        GetClientRect(GetDesktopWindow(), &desktop);
        int screenWidth = desktop.right;
        int screenHeight = desktop.bottom;

        const int windowWidth = 350;
        const int windowHeight = 100;

        std::uniform_int_distribution<> disX(0, screenWidth - windowWidth);
        std::uniform_int_distribution<> disY(0, screenHeight - windowHeight);

        int posX = disX(gen);
        int posY = disY(gen);

        HANDLE processHandle;
        HWND hWndChild = StartChildProcess(processNumber, lifeTime, posX, posY, hWnd, processHandle);
        if (hWndChild != NULL)
        {
            childWindows.push_back(hWndChild);
            childProcesses.push_back({ processNumber, lifeTime });
            processHandles.push_back(processHandle);
        }
        }).detach();
}

void StartProcesses(HWND hWnd, int count)
{
    targetProcessCount = count;

    for (int i = 0; i < count; ++i)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(3, 10);
        int lifeTime = dis(gen);

        StartNewProcess(hWnd, ++processCount, lifeTime);
    }
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
        case IDC_ENTERBUTTON:
        {
            BOOL success = FALSE;
            int count = GetDlgItemInt(hWnd, IDC_INPUTTEXT, &success, TRUE);

            if (success && count > 0)
            {
                SetErrorText(hWnd, L"", RGB(0, 0, 0));
                StartProcesses(hWnd, count);
            }
            else
            {
                SetErrorText(hWnd, L"Введите корректное целое число", RGB(255, 0, 0));
            }
            break;
        }
        case IDC_TERMINATEBUTTON:
        {
            for (HANDLE processHandle : processHandles)
            {
                TerminateProcess(processHandle, 0);
                CloseHandle(processHandle);
            }
            processHandles.clear();

            DestroyWindow(hWnd);
            break;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_CHILD_COMPLETED:
    {
        int completedProcessNumber = (int)wParam;
        for (size_t i = 0; i < childProcesses.size(); ++i)
        {
            if (childProcesses[i].first == completedProcessNumber)
            {
                CloseHandle(processHandles[i]);
                processHandles.erase(processHandles.begin() + i);
                HWND hWndChild = childWindows[i];
                childWindows.erase(childWindows.begin() + i);
                childProcesses.erase(childProcesses.begin() + i);

                PostMessage(hWndChild, WM_CLOSE, 0, 0);

                break;
            }
        }

        if (childProcesses.size() < targetProcessCount)
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(3, 10);
            int lifeTime = dis(gen);

            StartNewProcess(hWnd, ++processCount, lifeTime);
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PARENTPROCESS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PARENTPROCESS));

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
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PARENTPROCESS));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_PARENTPROCESS);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, 100, 100, 700, 200, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    CreateWindowW(L"STATIC", L"Введите количество процессов:", WS_VISIBLE | WS_CHILD, 10, 10, 200, 20, hWnd, NULL, hInst, NULL);
    CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 220, 10, 100, 20, hWnd, (HMENU)IDC_INPUTTEXT, hInst, NULL);
    CreateWindowW(L"BUTTON", L"Запустить", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 330, 10, 100, 20, hWnd, (HMENU)IDC_ENTERBUTTON, hInst, NULL);
    CreateWindowW(L"BUTTON", L"Завершить процесс", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 450, 10, 150, 20, hWnd, (HMENU)IDC_TERMINATEBUTTON, hInst, NULL);
    CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 10, 40, 400, 20, hWnd, (HMENU)IDC_ERRORTEXT, hInst, NULL);

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
