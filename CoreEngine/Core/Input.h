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

typedef struct __declspec(align(16)) SInput
{
	char keyButtons[512];
	char mouseButtons[8]; // 0 -> Left || 1 -> Right || 2 -> Middle || Handle Additional Buttons ...
	Vector2 v2MousePosition;
	Vector2 v2MouseDelta;
	float mouseScroll;
	bool bFirstMouseMove;
} SInput;

typedef struct SInput* Input;

bool InitializeInput(Input* ppInput);
void DestroyInput(Input* ppInput);
Input GetInput();

void UpdateInput(Input pInput);

// Keyboard Logic
void OnKeyButton(Input pInput, int key, int action);
bool IsKeyPressed(Input pInput, int key);
bool IsKeyDown(Input pInput, int key);
bool IsKeyReleased(Input pInput, int key);
bool IsKeyUp(Input pInput, int key);

// Mouse Buttons Logic
void OnMouseButton(Input pInput, int key, int action);
bool IsMouseButtonPressed(Input pInput, int key);
bool IsMouseButtonDown(Input pInput, int key);
bool IsMouseButtonReleased(Input pInput, int key);
bool IsMouseButtonUp(Input pInput, int key);

// Mouse Position Logic
void OnMousePosition(Input pInput, float xpos, float ypos);

// Mouse Scroll Logic
void OnMouseScroll(Input pInput, float yoffset);

void HandleKeys(Input pInput);
void HandleMouseButtons(Input pInput);
void HandleMouseMove(Input pInput);
void HandleMouseScroll(Input pInput);

#endif