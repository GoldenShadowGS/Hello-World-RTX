#pragma once
#include "PCH.h"
#include "Window.h"
#include "Renderer.h"

class Application
{
public:
	Application(HINSTANCE hInstance);
	~Application();
	int Run();
	void Update();
	void Render() ;
	void Exit() const;
private:
	void InitializeInput();
	void UpdateFrameTime();
	float GetFPS() const;
	void SetTitle() const;
	HINSTANCE m_hInstance;
	Window m_Window;
	Renderer m_Renderer;
	static Application* AppPtr;
	static LRESULT CALLBACK WindProcInit(HWND hwindow, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WindProc(HWND hwindow, UINT message, WPARAM wParam, LPARAM lParam);
	float m_FrameTime;
	WCHAR* m_TitleBuffer;
};
