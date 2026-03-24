#include "Input.h"
#include "Stdafx.h"

static Input ms_Input = NULL;

bool Input_Initialize(Input* ppInput)
{
    if (ppInput == NULL)
    {
        syserr("ppInput is NULL (invalid address)");
        return false;
    }

    // make sure it's all elements set to zero bytes
    *ppInput = engine_new_zero(SInput, 1, MEM_TAG_ENGINE);
    
    // Capture the dereferenced pointer for easier use in this function
    Input pInput = *ppInput;

    // Check if allocation failed
    if (!pInput)
    {
        syserr("Failed to Allocate Memory for Input");
        return (false);
    }

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
    engine_delete(pInput);

    *ppInput = NULL;
}

Input GetInput()
{
    return (ms_Input);
}

void Input_Update(Input pInput)
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

void Input_OnKeyButton(Input pInput, int key, int action)
{
    if (action == GLFW_PRESS)
    {
        pInput->keyButtons[key] = KEY_STATE_PRESSED;
    }
    else if (action == GLFW_RELEASE)
    {
        pInput->keyButtons[key] = KEY_STATE_RELEASED;
    }

    Input_HandleKeys(pInput);
}

bool Input_IsKeyPressed(Input pInput, int key)
{
    if (key >= 512 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    return pInput->keyButtons[key] == KEY_STATE_PRESSED;
}

bool Input_IsKeyDown(Input pInput, int key)
{
    if (key >= 512 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    bool IsDown = pInput->keyButtons[key] == KEY_STATE_PRESSED || pInput->keyButtons[key] == KEY_STATE_DOWN;
    return IsDown;
}

bool Input_IsKeyReleased(Input pInput, int key)
{
    if (key >= 512 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    return pInput->keyButtons[key] == KEY_STATE_RELEASED;
}

bool Input_IsKeyUp(Input pInput, int key)
{
    if (key >= 512 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    bool IsUp = pInput->keyButtons[key] == KEY_STATE_RELEASED || pInput->keyButtons[key] == KEY_STATE_UP;
    return IsUp;
}

void Input_OnMouseButton(Input pInput, int key, int action)
{
    if (action == GLFW_PRESS)
    {
        pInput->mouseButtons[key] = KEY_STATE_PRESSED;
    }
    else if (action == GLFW_RELEASE)
    {
        pInput->mouseButtons[key] = KEY_STATE_RELEASED;
    }

    Input_HandleMouseButtons(pInput);
}

bool Input_IsMouseButtonPressed(Input pInput, int key)
{
    if (key >= 8 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    return pInput->mouseButtons[key] == KEY_STATE_PRESSED;
}

bool Input_IsMouseButtonDown(Input pInput, int key)
{
    if (key >= 8 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    bool IsDown = pInput->mouseButtons[key] == KEY_STATE_PRESSED || pInput->mouseButtons[key] == KEY_STATE_DOWN;
    return IsDown;
}

bool Input_IsMouseButtonReleased(Input pInput, int key)
{
    if (key >= 8 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    return pInput->mouseButtons[key] == KEY_STATE_RELEASED;
}

bool Input_IsMouseButtonUp(Input pInput, int key)
{
    if (key >= 8 || key < 0)
    {
        syserr("Tried to Access out of bounds Key! (%d)", key);
        return (false);
    }

    bool IsUp = pInput->mouseButtons[key] == KEY_STATE_RELEASED || pInput->mouseButtons[key] == KEY_STATE_UP;
    return IsUp;
}

void Input_OnMousePosition(Input pInput, float xpos, float ypos)
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

    Input_HandleMouseMove();
}

void Input_OnMouseScroll(Input pInput, float yoffset)
{
    pInput->mouseScroll = yoffset;
    Input_HandleMouseScroll();
}

void Input_HandleKeys(Input pInput)
{
    if (Input_IsKeyDown(pInput, GLFW_KEY_ESCAPE))
    {
        glfwSetWindowShouldClose(Window_GetGLWindow(GetEngine()->window), GLFW_TRUE);
        GetEngine()->isRunning = false;
    }
    if (Input_IsKeyDown(pInput, GLFW_KEY_H))
    {
        MemoryManager_PrintData();
    }

    if (Input_IsKeyDown(pInput, GLFW_KEY_T))
    {
        MemoryManager_Validate();
    }
    if (Input_IsKeyDown(pInput, GLFW_KEY_J))
    {
        MemoryManager_PrintTagReport();
    }

    if (Input_IsKeyDown(pInput, GLFW_KEY_L))
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

void Input_HandleMouseButtons(Input pInput)
{
    if (Input_IsMouseButtonDown(pInput, GLFW_MOUSE_BUTTON_RIGHT))
    {
        // Disable Cursor for free rotation
        glfwSetInputMode(Window_GetGLWindow(GetEngine()->window), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    if (Input_IsMouseButtonDown(pInput, GLFW_MOUSE_BUTTON_LEFT))
    {
        // Enable Cursor
        glfwSetInputMode(Window_GetGLWindow(GetEngine()->window), GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
    }
}

void Input_HandleMouseMove()
{
    Camera_ProcessCameraMouse(GetEngine()->camera);
}

void Input_HandleMouseScroll()
{
    Camera_ProcessCameraZoom(GetEngine()->camera);
}

