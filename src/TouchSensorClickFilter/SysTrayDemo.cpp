// SysTrayDemo.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"

#include "HookDll.h"

#include "helpers/EventReporter.h"
#include "helpers/SimpleBuffer.h"

#define MAX_LOADSTRING 100
#define	WM_USER_SHELLICON WM_USER + 1

// Global Variables:
HINSTANCE      hInst;
NOTIFYICONDATA nidApp;

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance, const TCHAR* szWindowClass);
BOOL InitInstance(HINSTANCE hInstance, const TCHAR* szWindowClass, const TCHAR* szTitle);

// Windows API callbacks
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);


static void StopEvent(const TCHAR* reason)
{
    TCHAR szBuffer[128];
    _sntprintf(szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0]), _T("%s. %u clicks and %u moves eliminated."), reason, HookDll::Clicks(), HookDll::Moves());
    EventReporter::Instance().Report(szBuffer, EventReporter::eStop);
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    TCHAR szTitle[MAX_LOADSTRING];       // The title bar text
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

    TCHAR szWindowClass[MAX_LOADSTRING]; // The main window class name
    LoadString(hInstance, IDC_APP_CLASS, szWindowClass, MAX_LOADSTRING);

    EventReporter reporter(szWindowClass);

    // Only one instance is allowed:
    if (FindWindow(szWindowClass, szTitle) != NULL)
    {
        reporter.Report(_T("Already running, quit"), EventReporter::eAlreadyRunning, EVENTLOG_WARNING_TYPE);
        return 0;
    }

    static const TCHAR szDetached[] = _T("detached");
    if (lpCmdLine[0] == 0 || _tcsstr(lpCmdLine, _T("daemonize")) != NULL)
    { // "Fork" itself and quit
        TCHAR const* pgmptr =
#ifdef _UNICODE
            _wpgmptr;
#else
            _pgmptr;
#endif
        const size_t pathLength = _tcslen(pgmptr);
        SimpleBuffer<TCHAR> cmdLine(pathLength + 1 + sizeof(szDetached) / sizeof(szDetached[0]));
        cmdLine.Append(pgmptr, pathLength);
        cmdLine.Append(_T(' '));
        cmdLine.Append(szDetached, sizeof(szDetached) / sizeof(szDetached[0]));

        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (CreateProcess(NULL, cmdLine.Data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            reporter.Report(_T("Daemonizing"), EventReporter::eDaemonize);
        }
        else
            reporter.Report(_T("Failed to daemonize, quit"), EventReporter::eGeneric, EVENTLOG_ERROR_TYPE);
        return 0;
    }
    else if (_tcsstr(lpCmdLine, szDetached) != NULL)
    {
        // Continue working
    }
    else
    {
        TCHAR szText[MAX_LOADSTRING] = { 0, };
        LoadString(hInstance, IDS_UNKNOWN_CMDLINE, szText, MAX_LOADSTRING);
        SimpleBuffer<TCHAR> szMessage(_tcslen(szText) + _tcslen(lpCmdLine) + 1);
        szMessage.Append(szText, _tcslen(szText));
        szMessage.Append(lpCmdLine, _tcslen(lpCmdLine));
        szMessage.Append(_T('\0'));
        MessageBox(NULL, szMessage.Data(), szTitle, MB_OK | MB_ICONSTOP);
        return 1;
    }

    MyRegisterClass(hInstance, szWindowClass);

    // Perform application initialization:
    if (!InitInstance(hInstance, szWindowClass, szTitle))
    {
        reporter.Report(_T("InitInstance failed, quit"), EventReporter::eGeneric, EVENTLOG_ERROR_TYPE);
        return 0;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_APP_CLASS));

    reporter.Report(_T("Started"), EventReporter::eStart);

    HookDll hook; // Install mouse hook

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    StopEvent(_T("Stopped"));

    return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance, const TCHAR* szWindowClass)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName  = MAKEINTRESOURCE(IDC_APP_CLASS);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
#define IDI_APP_TRAY_ICON_ID 108 // Don't change its value!
BOOL InitInstance(HINSTANCE hInstance, const TCHAR* szWindowClass, const TCHAR* szTitle)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowEx(0, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (!hWnd) return FALSE;

    nidApp.cbSize = sizeof(NOTIFYICONDATA);           // sizeof the struct in bytes 
    nidApp.hWnd   = hWnd;                             // handle of the window which will process this app. messages 
    nidApp.uID    = IDI_APP_TRAY_ICON_ID;             // ID of the icon that will appear in the system tray 
    nidApp.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP; // ORing of all the flags 
    nidApp.hIcon  = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APP_ICON));
    nidApp.uCallbackMessage = WM_USER_SHELLICON;
    LoadString(hInstance, IDS_APPTOOLTIP_ENABLED, nidApp.szTip, sizeof(nidApp.szTip)/sizeof(nidApp.szTip[0]));
    Shell_NotifyIcon(NIM_ADD, &nidApp);

    return TRUE;
}

static void SetState(HINSTANCE hInstance, NOTIFYICONDATA& nidApp, bool disabled)
{
    DestroyIcon(nidApp.hIcon);
    nidApp.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(disabled ? IDI_APP_ICON_DISABLED : IDI_APP_ICON));
    LoadString(hInstance, disabled ? IDS_APPTOOLTIP_DISABLED : IDS_APPTOOLTIP_ENABLED, nidApp.szTip, sizeof(nidApp.szTip) / sizeof(nidApp.szTip[0]));
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_USER_SHELLICON:
        // Systray msg callback 
        switch (LOWORD(lParam))
        {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            POINT clickPoint;
            GetCursorPos(&clickPoint);
            HMENU hPopMenu = GetSubMenu(GetMenu(hWnd), 0);
            if (HookDll::Disabled())
                ::CheckMenuItem(hPopMenu, IDM_ENABLED, MF_UNCHECKED | MF_BYCOMMAND);
            else
                ::CheckMenuItem(hPopMenu, IDM_ENABLED, MF_CHECKED | MF_BYCOMMAND);
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, clickPoint.x, clickPoint.y, 0, hWnd, NULL);
            return TRUE;
        }
        break;
    case WM_COMMAND:
        // Parse the menu selections:
        switch (LOWORD(wParam))
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_ENABLED:
            HookDll::Enable(HookDll::Disabled());
            SetState(hInst, nidApp, HookDll::Disabled());
            Shell_NotifyIcon(NIM_MODIFY, &nidApp);
            break;
        case IDM_EXIT:
            Shell_NotifyIcon(NIM_DELETE, &nidApp);
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_DISPLAYCHANGE:
        Shell_NotifyIcon(NIM_DELETE, &nidApp);
        SetState(hInst, nidApp, HookDll::Disabled());
        Shell_NotifyIcon(NIM_ADD, &nidApp);
        return DefWindowProc(hWnd, message, wParam, lParam);
    /*
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code here...
        EndPaint(hWnd, &ps);
        break;
    */
    case WM_ENDSESSION:
        StopEvent(_T("Session ends"));
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for About box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        if (LOWORD(wParam) == IDC_URL)
        {
            TCHAR text[256];
            GetDlgItemText(hDlg, IDC_URL, text, sizeof(text) / sizeof(text[0]));
            ShellExecute(NULL, _T("open"), text, NULL, NULL, SW_SHOW);
            return TRUE;
        }
        break;
    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam == GetDlgItem(hDlg, IDC_URL))
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 255));
            return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
        }
        break;
    }
    return FALSE;
}
