#pragma once
#include "PCH.h"

class Application;

class Window
{
public:
	Window();
	~Window();
	void Create(HINSTANCE hInstance, Application* ptrApp, std::wstring title, UINT width, UINT height, bool fullscreen, WNDPROC wndproc);
	void ToggleFullScreen();
	void ResizedWindow();
	void Show();
	inline HWND GetHandle() const { return m_hWin; }
	inline UINT GetWindowWidth() const { return WindowWidth; }
	inline UINT GetWindowHeight() const { return WindowHeight; }
	inline UINT GetClientWidth() const { return ClientWidth; }
	inline UINT GetClientHeight() const { return ClientHeight; }
protected:
	void SetFullScreen(bool fullscreen);
	HINSTANCE m_Instance;
	HWND m_hWin;
	RECT m_WinRect;
	UINT WindowWidth;
	UINT WindowHeight;
	UINT ClientWidth;
	UINT ClientHeight;
	bool m_isFullScreen;
};
