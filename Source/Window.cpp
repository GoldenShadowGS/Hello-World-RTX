#include "PCH.h"
#include "Window.h"

#define FULLSCREEN_STYLE WS_OVERLAPPED
#define WINDOWED_STYLE WS_OVERLAPPEDWINDOW

#define WINDOWCLASSNAME L"DX12WindowClass"

ATOM RegisterWindowClass(HINSTANCE instance, std::wstring windowClassName, WNDPROC wndproc)
{
	WNDCLASSEXW wndClass = {};
	wndClass.cbSize = sizeof(WNDCLASSEXW);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = wndproc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = instance;
	wndClass.hIcon = LoadIcon(instance, IDI_APPLICATION);
	wndClass.hIconSm = LoadIcon(instance, IDI_APPLICATION);;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = windowClassName.c_str();
	return RegisterClassEx(&wndClass);
}

RECT CenterRect(RECT rect)
{
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	int X = std::max(0, GetSystemMetrics(SM_CXSCREEN) / 2 - width / 2);
	int Y = std::max(0, GetSystemMetrics(SM_CYSCREEN) / 2 - height / 2);

	return { X, Y, X + width, Y + height };
}

Window::Window() : m_Instance { }, m_hWin(nullptr), m_WinRect { }, WindowWidth(0), WindowHeight(0), ClientWidth(0), ClientHeight(0), m_isFullScreen(false)
{}

Window::~Window()
{
	DestroyWindow(m_hWin);
	UnregisterClass(WINDOWCLASSNAME, m_Instance);
}

void Window::Create(HINSTANCE hInstance, Application* ptrApp, std::wstring title, UINT width, UINT height, bool fullscreen, WNDPROC wndproc)
{
	m_Instance = hInstance;
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	RegisterWindowClass(hInstance, WINDOWCLASSNAME, wndproc);
	{
		RECT rect = { 0, 0, (LONG)width, (LONG)height };
		AdjustWindowRect(&rect, WINDOWED_STYLE, false);
		m_WinRect = CenterRect(rect);
	}
	const int Width = m_WinRect.right - m_WinRect.left;
	const int Height = m_WinRect.bottom - m_WinRect.top;
	m_hWin = CreateWindowExW(0, WINDOWCLASSNAME, title.c_str(), WINDOWED_STYLE, m_WinRect.left, m_WinRect.top, Width, Height, nullptr, nullptr, m_Instance, ptrApp);
	if (!m_hWin)
	{
		throw std::exception("Could not Create Window");
	}
	ResizedWindow();
	if (fullscreen)
	{
		SetFullScreen(true);
	}
}

void Window::ToggleFullScreen()
{
	SetFullScreen(!m_isFullScreen);
}

void Window::SetFullScreen(bool fullscreen)
{
	if (fullscreen)
	{
		m_isFullScreen = true;
		// Save Window Position Rect
		GetWindowRect(m_hWin, &m_WinRect);
		SetWindowLongPtrW(m_hWin, GWL_STYLE, FULLSCREEN_STYLE);
		SetWindowPos(m_hWin, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED | SWP_NOACTIVATE);
		ShowWindow(m_hWin, SW_SHOWMAXIMIZED);
	}
	else
	{
		m_isFullScreen = false;
		const int width = m_WinRect.right - m_WinRect.left;
		const int height = m_WinRect.bottom - m_WinRect.top;
		SetWindowLongPtrW(m_hWin, GWL_STYLE, WINDOWED_STYLE);
		SetWindowPos(m_hWin, NULL, m_WinRect.left, m_WinRect.top, width, height, SWP_FRAMECHANGED | SWP_NOACTIVATE);
		ShowWindow(m_hWin, SW_NORMAL);
	}
}

void Window::ResizedWindow()
{
	RECT rect = {};
	GetWindowRect(m_hWin, &rect);
	WindowWidth = rect.right - rect.left;
	WindowHeight = rect.bottom - rect.top;

	GetClientRect(m_hWin, &rect);
	ClientWidth = rect.right - rect.left;
	ClientHeight = rect.bottom - rect.top;
}

void Window::Show()
{
	ShowWindow(m_hWin, SW_SHOW);
}