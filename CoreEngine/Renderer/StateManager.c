#include "StateManager.h"
#include "../Core/CoreUtils.h"
#include "../Engine.h"
#include <memory.h>

static StateManager ms_StateManager = NULL;

bool InitializeStateManager(StateManager* ppStateManager)
{
	*ppStateManager = (StateManager)tracked_malloc(sizeof(SStateManager));

	StateManager stateManager = *ppStateManager;
	if (!stateManager)
	{
		return (false);
	}

	ms_StateManager = stateManager;

	stateManager->top = 0;
	stateManager->pCurrentStack = &stateManager->stateStack[0];
	stateManager->pCurrentStack->pCurrentBuffer = NULL;
	stateManager->pCurrentStack->pCurrentShader = NULL;

	stateManager->pCurrentStack->viewport[0] = 0;
	stateManager->pCurrentStack->viewport[1] = 0;
	stateManager->pCurrentStack->viewport[2] = GetWindowWidth(GetEngine()->window);
	stateManager->pCurrentStack->viewport[3] = GetWindowHeight(GetEngine()->window);

	stateManager->pCurrentStack->scissorBox[0] = 0;
	stateManager->pCurrentStack->scissorBox[1] = 0;
	stateManager->pCurrentStack->scissorBox[2] = GetWindowWidth(GetEngine()->window);
	stateManager->pCurrentStack->scissorBox[3] = GetWindowHeight(GetEngine()->window);

	stateManager->pCurrentStack->blendSrc = GL_ONE;
	stateManager->pCurrentStack->blendDst = GL_ZERO;

	stateManager->pCurrentStack->depthFunc = GL_LESS;
	stateManager->pCurrentStack->frontFace = GL_CCW;
	stateManager->pCurrentStack->cullFace = GL_BACK;

	stateManager->pCurrentStack->enabledCapabilities = CAP_DEPTH_TEST | CAP_CULL_FACE;

	stateManager->activeGPUSurface = *stateManager->pCurrentStack;
	return (true);
}

void PushState(StateManager pManager)
{
	if (pManager->top >= MAX_STACKS_ALLOWED - 1)
	{
		return;
	}

	// Copy everything from the current level to the next level
	pManager->stateStack[pManager->top + 1] = pManager->stateStack[pManager->top];

	pManager->top++;

	// Update the pointer to the new top slot!
	pManager->pCurrentStack = &pManager->stateStack[pManager->top];
}

void PopState(StateManager pManager)
{
	if (pManager->top <= 0)
	{
		syserr("State Stack Underflow! You called Pop too many times.");
		return;
	}

	// Just move the pointer back.
	// The state at [top - 1] is already your "saved" state.
	pManager->top--;

	pManager->pCurrentStack = &pManager->stateStack[pManager->top]; // Move pointer down

	// Tell OpenGL to match the newly restored state
	ApplyState(pManager, GetCurrentState(pManager));
}

void BindShader(StateManager pManager, GLShader pShader)
{
	// Update the Desired State in the stack
	pManager->pCurrentStack->pCurrentShader = pShader;

	// Immediately sync the GPU with the current stack
	ApplyState(pManager, pManager->pCurrentStack);
}

void BindBufferVAO(StateManager pManager, GLBuffer pBuffer)
{
	pManager->pCurrentStack->pCurrentBuffer = pBuffer;

	// Immediately sync the GPU with the current stack
	ApplyState(pManager, pManager->pCurrentStack);
}

void SetViewport(StateManager pManager, int x, int y, int width, int height)
{
	// Update the Desired State in the stack
	pManager->pCurrentStack->viewport[0] = x;
	pManager->pCurrentStack->viewport[1] = y;
	pManager->pCurrentStack->viewport[2] = width;
	pManager->pCurrentStack->viewport[3] = height;

	// Immediately sync the GPU with the current stack
	ApplyState(pManager, pManager->pCurrentStack);
}

void SetScissorRect(StateManager pManager, int x, int y, int width, int height)
{
	// Update the Desired State in the stack
	pManager->pCurrentStack->scissorBox[0] = x;
	pManager->pCurrentStack->scissorBox[1] = y;
	pManager->pCurrentStack->scissorBox[2] = width;
	pManager->pCurrentStack->scissorBox[3] = height;

	// Immediately sync the GPU with the current stack
	ApplyState(pManager, pManager->pCurrentStack);
}

void SetBlendFunc(StateManager pManager, GLenum src, GLenum dst)
{
	pManager->pCurrentStack->blendSrc = src;
	pManager->pCurrentStack->blendDst = dst;

	// Immediately sync the GPU with the current stack
	ApplyState(pManager, pManager->pCurrentStack);
}

void SetCapability(StateManager pManager, EEngineCap cap, bool bEnable)
{
	if (bEnable)
	{
		pManager->pCurrentStack->enabledCapabilities |= cap; // Set bit to 1
	}
	else
	{
		pManager->pCurrentStack->enabledCapabilities &= ~cap; // Set bit to 0
	}

	// Apply the change immediately to OpenGL
	ApplyState(pManager, pManager->pCurrentStack);
}

void ApplyState(StateManager pManager, StateSnapshot pNewStateSnap)
{
	// We compare against the state we had BEFORE the push/pop 
	// or just apply everything if you don't track the 'Previous' state.

	// 1. Handle Capabilities (The Bitmask)
	// For simplicity, we'll check the bits. 
	// Optimization: You can use the 'changed' XOR.
	SStateSnapshot* pActive = &pManager->activeGPUSurface;

	// 1. Capabilities (XOR is the most efficient way to do this)
	uint32_t changed = pActive->enabledCapabilities ^ pNewStateSnap->enabledCapabilities;
	if (changed & CAP_DEPTH_TEST)
	{
		if (pNewStateSnap->enabledCapabilities & CAP_DEPTH_TEST)
		{
			glEnable(GL_DEPTH_TEST);
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}
	}
	if (changed & CAP_CULL_FACE)
	{
		if (pNewStateSnap->enabledCapabilities & CAP_CULL_FACE)
		{
			glEnable(GL_CULL_FACE);
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}
	}

	// 2. Viewport & Scissor (memcmp is great here)
	if (memcmp(pNewStateSnap->viewport, pActive->viewport, sizeof(int) * 4) != 0)
	{
		glViewport(pNewStateSnap->viewport[0], pNewStateSnap->viewport[1], pNewStateSnap->viewport[2], pNewStateSnap->viewport[3]);
	}
	if (memcmp(pNewStateSnap->scissorBox, pActive->scissorBox, sizeof(int) * 4) != 0)
	{
		glScissor(pNewStateSnap->scissorBox[0], pNewStateSnap->scissorBox[1], pNewStateSnap->scissorBox[2], pNewStateSnap->scissorBox[3]);
	}

	/// Blend Func
	if (pNewStateSnap->blendSrc != pActive->blendSrc || pNewStateSnap->blendDst != pActive->blendDst)
	{
		glBlendFunc(pNewStateSnap->blendSrc, pNewStateSnap->blendDst);
	}

	// 3. Shader & Buffer (The "Heavy" switches)
	if (pNewStateSnap->pCurrentShader != pActive->pCurrentShader)
	{
		if (pNewStateSnap->pCurrentShader != NULL)
		{
			UseProgram(pNewStateSnap->pCurrentShader);
		}
		else
		{
			glUseProgram(0); // Unbind
		}
	}
	if (pNewStateSnap->pCurrentBuffer != pActive->pCurrentBuffer)
	{
		if (pNewStateSnap->pCurrentBuffer != NULL)
		{
			glBindVertexArray(GetVertexArray(pNewStateSnap->pCurrentBuffer));
		}
		else
		{
			glBindVertexArray(0); // Unbind
		}
	}

	// Always update the tracker so the NEXT ApplyState knows what happened
	pManager->activeGPUSurface = *pNewStateSnap;
}

StateSnapshot GetCurrentState(StateManager pManager)
{
	return pManager->pCurrentStack;
}

int GetStateDepth(StateManager pManager)
{
	return pManager->top;
}

StateManager GetStateManager()
{
	return (ms_StateManager);
}

void DestroyStateManager(StateManager* ppManager)
{
	if (!ppManager || !*ppManager)
	{
		return;
	}

	StateManager pManager = *ppManager;
	tracked_free(pManager);

	*ppManager = NULL;
}
