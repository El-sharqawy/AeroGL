#include "Input.h"
#include "CoreUtils.h"
#include <memory.h>
#include <GLFW/glfw3.h>
#include "../Engine.h"

static Input ms_Input = NULL;

bool Input_Initialize(Input* ppInput)
{
    if (ppInput == NULL)
    {
        syserr("ppInput is NULL (invalid address)");
        return false;
    }

    *ppInput = tracked_malloc(sizeof(SInput));
    
    // Capture the dereferenced pointer for easier use in this function
    Input pInput = *ppInput;

    // Check if allocation failed
    if (!pInput)
    {
        syserr("Failed to Allocate Memory for Input");
        return (false);
    }

    // Initialize everything to zero
    memset(pInput, 0, sizeof(SInput));

    pInput->bFirstMouseMove = true;

    ms_Input = pInput;
    return (true);
}

void Input_Destroy(Input* ppInput)
{
    if (!ppInput || !*ppInput)
    {
        return;
    }

    Input pInput = *ppInput;
    tracked_free(pInput);

    *ppInput = NULL;
}

Input GetInput()
{
    return (ms_Input);
}

void UpdateInput(Input pInput)
{
    // Keyboard Buttons States Logic
    for (int i = 0; i < 512; i++)
    {
        if (pInput->keyButtons[i] == KEY_STATE_PRESSED)
        {
            pInput->keyButtons[i] = KEY_STATE_DOWN;
        }

        if (pInput->keyButtons[i] == KEY_STATE_RELEASED)
        {
            pInput->keyButtons[i] = KEY_STATE_UP;
        }
    }

    // Mouse Buttons States Logic
    for (int i = 0; i < 8; i++)
    {
        if (pInput->mouseButtons[i] == KEY_STATE_PRESSED)
        {
            pInput->mouseButtons[i] = KEY_STATE_DOWN;
        }

        if (pInput->mouseButtons[i] == KEY_STATE_RELEASED)
        {
            pInput->mouseButtons[i] = KEY_STATE_UP;
        }
    }

    // Reset delta so the camera stops moving when the mouse stops moving
    pInput->v2MouseDelta.x = 0.0f;
    pInput->v2MouseDelta.y = 0.0f;
    pInput->mouseScroll = 0.0f;
}

void OnKeyButton(Input pInput, int key, int action)
{
    if (action == GLFW_PRESS)
    {
        pInput->keyButtons[key] = KEY_STATE_PRESSED;
    }
    else if (action == GLFW_RELEASE)
    {
        pInput->keyButtons[key] = KEY_STATE_RELEASED;
    }

    HandleKeys(pInput);
}

bool IsKeyPressed(Input pInput, int key)
{
    if (key >= 512 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    return pInput->keyButtons[key] == KEY_STATE_PRESSED;
}

bool IsKeyDown(Input pInput, int key)
{
    if (key >= 512 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    bool IsDown = pInput->keyButtons[key] == KEY_STATE_PRESSED || pInput->keyButtons[key] == KEY_STATE_DOWN;
    return IsDown;
}

bool IsKeyReleased(Input pInput, int key)
{
    if (key >= 512 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    return pInput->keyButtons[key] == KEY_STATE_RELEASED;
}

bool IsKeyUp(Input pInput, int key)
{
    if (key >= 512 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    bool IsUp = pInput->keyButtons[key] == KEY_STATE_RELEASED || pInput->keyButtons[key] == KEY_STATE_UP;
    return IsUp;
}

void OnMouseButton(Input pInput, int key, int action)
{
    if (action == GLFW_PRESS)
    {
        pInput->mouseButtons[key] = KEY_STATE_PRESSED;
    }
    else if (action == GLFW_RELEASE)
    {
        pInput->mouseButtons[key] = KEY_STATE_RELEASED;
    }

    HandleMouseButtons(pInput);
}

bool IsMouseButtonPressed(Input pInput, int key)
{
    if (key >= 8 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    return pInput->mouseButtons[key] == KEY_STATE_PRESSED;
}

bool IsMouseButtonDown(Input pInput, int key)
{
    if (key >= 8 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    bool IsDown = pInput->mouseButtons[key] == KEY_STATE_PRESSED || pInput->mouseButtons[key] == KEY_STATE_DOWN;
    return IsDown;
}

bool IsMouseButtonReleased(Input pInput, int key)
{
    if (key >= 8 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    return pInput->mouseButtons[key] == KEY_STATE_RELEASED;
}

bool IsMouseButtonUp(Input pInput, int key)
{
    if (key >= 8 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    bool IsUp = pInput->mouseButtons[key] == KEY_STATE_RELEASED || pInput->mouseButtons[key] == KEY_STATE_UP;
    return IsUp;
}

void OnMousePosition(Input pInput, float xpos, float ypos)
{
    if (pInput->bFirstMouseMove)
    {
        pInput->v2MousePosition.x = xpos;
        pInput->v2MousePosition.y = ypos;
        pInput->bFirstMouseMove = false;
        return;
    }

    // 1. Calculate the distance moved
    pInput->v2MouseDelta.x = xpos - pInput->v2MousePosition.x;
    pInput->v2MouseDelta.y = pInput->v2MousePosition.y - ypos; // Inverted for standard Pitch logic

    // 2. Store the current position for the next frame's calculation
    pInput->v2MousePosition = Vector2D(xpos, ypos);

    HandleMouseMove(pInput);
}

void OnMouseScroll(Input pInput, float yoffset)
{
    pInput->mouseScroll = yoffset;
    HandleMouseScroll(pInput);
}

void HandleKeys(Input pInput)
{
    if (IsKeyDown(pInput, GLFW_KEY_ESCAPE))
    {
        glfwSetWindowShouldClose(GetGLWindow(GetEngine()->window), GLFW_TRUE);
        GetEngine()->isRunning = false;
    }
    if (IsKeyDown(pInput, GLFW_KEY_H))
    {
        syslog("Currently Allocated: %zu Objects with size of %0.2f Kilo Bytes", allocation_count, (double)bytes_allocated / 1024.0);
    }
    if (IsKeyDown(pInput, GLFW_KEY_L))
    {
        if (!GetEngine()->isWireframe)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            GetEngine()->isWireframe = true;
        }
        else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            GetEngine()->isWireframe = false;
        }
    }
}

void HandleMouseButtons(Input pInput)
{
    if (IsMouseButtonDown(pInput, GLFW_MOUSE_BUTTON_RIGHT))
    {
        // Disable Cursor for free rotation
        glfwSetInputMode(GetGLWindow(GetEngine()->window), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    if (IsMouseButtonDown(pInput, GLFW_MOUSE_BUTTON_LEFT))
    {
        // Enable Cursor
        glfwSetInputMode(GetGLWindow(GetEngine()->window), GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
    }
}

void HandleMouseMove(Input pInput)
{
    ProcessCameraMouse(GetEngine()->camera);
}

void HandleMouseScroll(Input pInput)
{
    ProcessCameraZoom(GetEngine()->camera);
}

