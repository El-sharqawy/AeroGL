#include "StateManager.h"
#include "../Stdafx.h"

bool StateManager_Initialize(StateManager* ppStateManager)
{
	if (ppStateManager == NULL)
	{
		syserr("ppStateManager is NULL (invalid address)");
		return false;
	}

	*ppStateManager = engine_new(SStateManager, MEM_TAG_RENDERING);

	StateManager stateManager = *ppStateManager;
	if (!stateManager)
	{
		return (false);
	}

	psStateManager = stateManager;

	stateManager->top = 0;
	stateManager->pCurrentStack = &stateManager->stateStack[0];
	stateManager->pCurrentStack->uiCurrentVAO = 0;
	stateManager->pCurrentStack->pCurrentShader = NULL;

	stateManager->pCurrentStack->viewport[0] = 0;
	stateManager->pCurrentStack->viewport[1] = 0;
	stateManager->pCurrentStack->viewport[2] = Window_GetWidth(GetEngine()->window);
	stateManager->pCurrentStack->viewport[3] = Window_GetHeight(GetEngine()->window);

	stateManager->pCurrentStack->scissorBox[0] = 0;
	stateManager->pCurrentStack->scissorBox[1] = 0;
	stateManager->pCurrentStack->scissorBox[2] = Window_GetWidth(GetEngine()->window);
	stateManager->pCurrentStack->scissorBox[3] = Window_GetHeight(GetEngine()->window);

	stateManager->pCurrentStack->blendSrc = GL_ONE;
	stateManager->pCurrentStack->blendDst = GL_ZERO;

	stateManager->pCurrentStack->depthFunc = GL_LESS;
	stateManager->pCurrentStack->depthMask = GL_TRUE; // Usually the default

	stateManager->pCurrentStack->frontFace = GL_CCW;
	stateManager->pCurrentStack->cullFace = GL_BACK;

	stateManager->pCurrentStack->enabledCapabilities = CAP_DEPTH_TEST | CAP_CULL_FACE;

	stateManager->activeGPUSurface = *stateManager->pCurrentStack;
	return (true);
}

void StateManager_Destroy(StateManager* ppManager)
{
	if (!ppManager || !*ppManager)
	{
		return;
	}

	StateManager pManager = *ppManager;
	engine_delete(pManager);

	*ppManager = NULL;
}

StateSnapshot StateManager_GetCurrentState(StateManager pManager)
{
	return pManager->pCurrentStack;
}

int32_t StateManager_GetStateDepth(StateManager pManager)
{
	return pManager->top;
}

void StateManager_PushState(StateManager pManager)
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

void StateManager_PopState(StateManager pManager)
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
	StateManager_ApplyState(pManager, StateManager_GetCurrentState(pManager));
}

void StateManager_BindShader(StateManager pManager, GLShader pShader)
{
	// Update the Desired State in the stack
	pManager->pCurrentStack->pCurrentShader = pShader;

	// Immediately sync the GPU with the current stack
	StateManager_ApplyState(pManager, pManager->pCurrentStack);
}

void StateManager_BindBufferVAO(StateManager pManager, GLBuffer pBuffer)
{
	pManager->pCurrentStack->uiCurrentVAO = Mesh3DGLBuffer_GetVertexArray(pBuffer);

	// Immediately sync the GPU with the current stack
	StateManager_ApplyState(pManager, pManager->pCurrentStack);
}

void StateManager_BindTerrainBufferVAO(StateManager pManager, TerrainGLBuffer pBuffer)
{
	pManager->pCurrentStack->uiCurrentVAO = TerrainBuffer_GetVertexArray(pBuffer);

	// Immediately sync the GPU with the current stack
	StateManager_ApplyState(pManager, pManager->pCurrentStack);
}

void StateManager_SetViewport(StateManager pManager, int x, int y, int width, int height)
{
	// Update the Desired State in the stack
	pManager->pCurrentStack->viewport[0] = x;
	pManager->pCurrentStack->viewport[1] = y;
	pManager->pCurrentStack->viewport[2] = width;
	pManager->pCurrentStack->viewport[3] = height;

	// Immediately sync the GPU with the current stack
	StateManager_ApplyState(pManager, pManager->pCurrentStack);
}

void StateManager_SetScissorRect(StateManager pManager, int x, int y, int width, int height)
{
	// Update the Desired State in the stack
	pManager->pCurrentStack->scissorBox[0] = x;
	pManager->pCurrentStack->scissorBox[1] = y;
	pManager->pCurrentStack->scissorBox[2] = width;
	pManager->pCurrentStack->scissorBox[3] = height;

	// Immediately sync the GPU with the current stack
	StateManager_ApplyState(pManager, pManager->pCurrentStack);
}

void StateManager_SetBlendFunc(StateManager pManager, GLenum src, GLenum dst)
{
	pManager->pCurrentStack->blendSrc = src;
	pManager->pCurrentStack->blendDst = dst;

	// Immediately sync the GPU with the current stack
	StateManager_ApplyState(pManager, pManager->pCurrentStack);
}

void StateManager_SetCapability(StateManager pManager, EEngineCap cap, bool bEnable)
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
	StateManager_ApplyState(pManager, pManager->pCurrentStack);
}

void StateManager_SetDepthFunc(StateManager pManager, GLenum eDepthFunc)
{
	pManager->pCurrentStack->depthFunc = eDepthFunc;

	// Apply the change immediately to OpenGL
	StateManager_ApplyState(pManager, pManager->pCurrentStack);
}

void StateManager_SetFrontFace(StateManager pManager, GLenum eFrontFace)
{
	pManager->pCurrentStack->frontFace = eFrontFace;

	// Apply the change immediately to OpenGL
	StateManager_ApplyState(pManager, pManager->pCurrentStack);
}

void StateManager_SetCullFace(StateManager pManager, GLenum eCullFace)
{
	pManager->pCurrentStack->cullFace = eCullFace;

	// Apply the change immediately to OpenGL
	StateManager_ApplyState(pManager, pManager->pCurrentStack);
}

void StateManager_ApplyState(StateManager pManager, StateSnapshot pNewStateSnap)
{
	// We compare against the state we had BEFORE the push/pop 
	// or just apply everything if you don't track the 'Previous' state.

	// 1. Handle Capabilities (The Bitmask)
	// For simplicity, we'll check the bits. 
	// Optimization: You can use the 'changed' XOR.
	StateSnapshot pActive = &pManager->activeGPUSurface;

	// 1. Toggles (Alpha Test, Depth Test, etc.)
	StateManager_ApplyCapabilities(pActive, pNewStateSnap);

	// 2. Geometry/Screen (Viewport, Scissor, Culling)
	StateManager_ApplyRasterizer(pActive, pNewStateSnap);

	// 3. Pixel Operations (Depth, Blend, etc)
	StateManager_ApplyDepthState(pActive, pNewStateSnap);
	StateManager_ApplyBlendState(pActive, pNewStateSnap);

	// 4. Shader & Buffer (The "Heavy" switches)
	StateManager_ApplyResources(pActive, pNewStateSnap);

	// Always update the tracker so the NEXT ApplyState knows what happened
	pManager->activeGPUSurface = *pNewStateSnap;
}

void StateManager_ApplyCapabilities(StateSnapshot pActiveState, StateSnapshot pNewState)
{
	// Capabilities (XOR is the most efficient way to do this)
	uint32_t changed = pActiveState->enabledCapabilities ^ pNewState->enabledCapabilities;
	if (!changed)
	{
		return;
	}

	if (changed & CAP_DEPTH_TEST)
	{
		if (pNewState->enabledCapabilities & CAP_DEPTH_TEST)
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
		if (pNewState->enabledCapabilities & CAP_CULL_FACE)
		{
			glEnable(GL_CULL_FACE);
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}
	}

	if (changed & CAP_BLEND)
	{
		if (pNewState->enabledCapabilities & CAP_BLEND)
		{
			glEnable(GL_BLEND);
		}
		else
		{
			glDisable(GL_BLEND);
		}
	}

	if (changed & CAP_SCISSOR_TEST)
	{
		if (pNewState->enabledCapabilities & CAP_SCISSOR_TEST)
		{
			glEnable(GL_SCISSOR_TEST);
		}
		else
		{
			glDisable(GL_SCISSOR_TEST);
		}
	}
}

void StateManager_ApplyRasterizer(StateSnapshot pActiveState, StateSnapshot pNewState)
{
	// if (memcmp(pNewState->viewport, pActiveState->viewport, sizeof(int) * 4) != 0) // this could be slower than if 
	if ((pNewState->viewport[0] != pActiveState->viewport[0]) || (pNewState->viewport[1] != pActiveState->viewport[1]) || (pNewState->viewport[2] != pActiveState->viewport[2]) || (pNewState->viewport[3] != pActiveState->viewport[3]))
	{
		glViewport(pNewState->viewport[0], pNewState->viewport[1], pNewState->viewport[2], pNewState->viewport[3]);
	}
	// if (memcmp(pNewState->scissorBox, pActiveState->scissorBox, sizeof(int) * 4) != 0) // this could be slower than if 
	if ((pNewState->scissorBox[0] != pActiveState->scissorBox[0]) || (pNewState->scissorBox[1] != pActiveState->scissorBox[1]) || (pNewState->scissorBox[2] != pActiveState->scissorBox[2]) || (pNewState->scissorBox[3] != pActiveState->scissorBox[3]))
	{
		glScissor(pNewState->scissorBox[0], pNewState->scissorBox[1], pNewState->scissorBox[2], pNewState->scissorBox[3]);
	}

	if (pNewState->frontFace != pActiveState->frontFace)
	{
		glFrontFace(pNewState->frontFace);
	}

	if (pNewState->cullFace != pActiveState->cullFace)
	{
		glCullFace(pNewState->cullFace);
	}
}

void StateManager_ApplyDepthState(StateSnapshot pActive, StateSnapshot pNewStateSnap)
{
	// Depth Function
	if (pNewStateSnap->depthFunc != pActive->depthFunc)
	{
		glDepthFunc(pNewStateSnap->depthFunc);
	}

	if (pNewStateSnap->depthMask != pActive->depthMask)
	{
		glDepthMask(pNewStateSnap->depthMask); // GL_TRUE or GL_FALSE
	}
}

void StateManager_ApplyBlendState(StateSnapshot pActive, StateSnapshot pNewStateSnap)
{
	/// Blend Func
	if (pNewStateSnap->blendSrc != pActive->blendSrc || pNewStateSnap->blendDst != pActive->blendDst)
	{
		glBlendFunc(pNewStateSnap->blendSrc, pNewStateSnap->blendDst);
	}
}

void StateManager_ApplyResources(StateSnapshot pActiveState, StateSnapshot pNewState)
{
	if (pActiveState->pCurrentShader != pNewState->pCurrentShader)
	{
		if (pNewState->pCurrentShader != NULL)
		{
			Shader_UseProgram(pNewState->pCurrentShader);
		}
		else
		{
			glUseProgram(0); // Unbind
		}
	}
	if (pActiveState->uiCurrentVAO != pNewState->uiCurrentVAO)
	{
		glBindVertexArray(pNewState->uiCurrentVAO);
	}
}

// Singleton
StateManager GetStateManager()
{
	return (psStateManager);
}
