#ifndef __INPUT_H__
#define __INPUT_H__

#include <stdbool.h>
#include "../Math/Vectors/Vector2.h"

typedef enum EKeyState
{
	KEY_STATE_UP, // Default State is 0 .. Up
	KEY_STATE_RELEASED,
	KEY_STATE_PRESSED,
	KEY_STATE_DOWN,
} EKeyState;

typedef struct AERO_ALIGN(16) SInput
{
	char keyButtons[512];
	char mouseButtons[8]; // 0 -> Left || 1 -> Right || 2 -> Middle || Handle Additional Buttons ...
	Vector2 v2MousePosition;
	Vector2 v2MouseDelta;
	float mouseScroll;
	bool bFirstMouseMove;
	char padding[3];
} SInput;

typedef struct SInput* Input;

bool Input_Initialize(Input* ppInput);
void Input_Destroy(Input* ppInput);
Input GetInput();

void Input_Update(Input pInput);

// Keyboard Logic
void Input_OnKeyButton(Input pInput, int key, int action);
bool IsKeyPressed(Input pInput, int key);
bool IsKeyDown(Input pInput, int key);
bool IsKeyReleased(Input pInput, int key);
bool Input_IsKeyUp(Input pInput, int key);

// Mouse Buttons Logic
void Input_OnMouseButton(Input pInput, int key, int action);
bool Input_IsMouseButtonPressed(Input pInput, int key);
bool Input_IsMouseButtonDown(Input pInput, int key);
bool Input_IsMouseButtonReleased(Input pInput, int key);
bool Input_IsMouseButtonUp(Input pInput, int key);

// Mouse Position Logic
void Input_OnMousePosition(Input pInput, float xpos, float ypos);

// Mouse Scroll Logic
void Input_OnMouseScroll(Input pInput, float yoffset);

void Input_HandleKeys(Input pInput);
void Input_HandleMouseButtons(Input pInput);
void Input_HandleMouseMove();
void Input_HandleMouseScroll();

#endif
