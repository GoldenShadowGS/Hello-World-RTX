#pragma once
#include "PCH.h"
#include "Window.h"

namespace Input
{
#define MAX_KEY_STATES 256
#define MAX_BUTTON_STATES 5
	struct InputState
	{
		BYTE key[MAX_KEY_STATES];
		BYTE button[MAX_BUTTON_STATES];
		int mouseX_Screen;
		int mouseY_Screen;
		float mouseXf;
		float mouseYf;
		float mouseXDeltaf;
		float mouseYDeltaf;
		int MouseDeltaAccumulatorX;
		int MouseDeltaAccumulatorY;
	};
	extern InputState state;

	inline void Update()
	{
		state.mouseXDeltaf = state.MouseDeltaAccumulatorX / 1500.0f;
		state.mouseYDeltaf = state.MouseDeltaAccumulatorY / 1500.0f;
		state.MouseDeltaAccumulatorX = 0;
		state.MouseDeltaAccumulatorY = 0;
	}

	inline void RawMouseAccumulator(int x, int y)
	{
		state.MouseDeltaAccumulatorX += x;
		state.MouseDeltaAccumulatorY += y;
	}

	inline void UpdateMouse(LPARAM lParam, Window* win)
	{
		state.mouseX_Screen = GET_X_LPARAM(lParam);
		state.mouseY_Screen = GET_Y_LPARAM(lParam);
	}

	inline void HideMouseCursor(bool hidden)
	{
		static POINT point = {};
		if (hidden)
		{
			ShowCursor(FALSE);
			GetCursorPos(&point);
		}
		else
		{
			ShowCursor(TRUE);
			SetCursorPos(point.x, point.y);
		}
	}

	inline void SetButtonPressedState(UINT button, bool isDown)
	{
		assert(button < MAX_BUTTON_STATES);
		Input::state.button[button] = isDown;
	}

	inline bool GetButtonState(UINT button)
	{
		assert(button < MAX_BUTTON_STATES);
		return Input::state.button[button];
	}

	inline void SetKeyPressedState(UINT VK_Key, bool isDown)
	{
		assert(VK_Key < MAX_KEY_STATES);
		Input::state.key[VK_Key] = isDown;
	}

	inline bool GetKeyState(UINT VK_Key)
	{
		assert(VK_Key < MAX_KEY_STATES);
		return Input::state.key[VK_Key];
	}

	inline float GetMouseXDelta()
	{
		return state.mouseXDeltaf;
	}

	inline float GetMouseYDelta()
	{
		return state.mouseYDeltaf;
	}

	inline void KillFocus()
	{
		state = {};
	}
}