#ifndef __STATE_MANAGER_H__
#define __STATE_MANAGER_H__

#define MAX_STACKS_ALLOWED 32

#define MAX_TEXTURE_UNITS 32
#define MAX_CAPABILITIES 16

#define MAX_VIEWPORTS 4
#define MAX_SCISSORS 4

#include <glad/glad.h>
#include "../Buffers/Buffer.h"
#include "../Buffers/TerrainBuffer.h"
#include "Shader.h"

typedef enum EEngineCap
{
	CAP_DEPTH_TEST		= (1 << 0),		// 0001
	CAP_CULL_FACE		= (1 << 1),		// 0010
	CAP_BLEND			= (1 << 2),		// 0100
	CAP_SCISSOR_TEST	= (1 << 3),		// 1000
} EEngineCap;

typedef struct SStateSnapshot
{
	// GLenum mEngineCapabilities[MAX_CAPABILITIES]; // Basic Toggle States (Enable/Disable) -- Disabled for now
	// GLBuffer pCurrentBuffer;
	GLuint uiCurrentVAO;
	GLShader pCurrentShader;

	// UI Specifics
	GLint scissorBox[MAX_SCISSORS];			// {x, y, width, height}
	GLint viewport[MAX_VIEWPORTS];			// {x, y, width, height}
	GLenum blendSrc, blendDst;				// How transparency is calculated

	// Depth
	GLenum depthFunc;						// GL_LESS | GL_LEQUAL | etc ..
	GLboolean depthMask;					// GL_TRUE | GL_FALSE

	// FaceMask
	GLenum frontFace;						// GL_CCW | GL_CW
	GLenum cullFace;						// GL_BACK | GL_FRONT

	// Bitmask for GL_DEPTH_TEST, GL_BLEND, GL_CULL_FACE, GL_SCISSOR_TEST
	GLuint enabledCapabilities;
} SStateSnapshot;

typedef struct SStateSnapshot* StateSnapshot;

typedef struct SStateManager
{
	StateSnapshot pCurrentStack;					// Pointer to the active slot
	SStateSnapshot activeGPUSurface;				// Tracker for smart diffing
	SStateSnapshot stateStack[MAX_STACKS_ALLOWED];
	int32_t top;										// Index of the current active state
} SStateManager;

typedef struct SStateManager* StateManager;

bool StateManager_Initialize(StateManager* ppStateManager);
void StateManager_Destroy(StateManager* ppManager);

StateSnapshot StateManager_GetCurrentState(StateManager pManager);
int32_t StateManager_GetStateDepth(StateManager pManager);

void StateManager_PushState(StateManager pManager);
void StateManager_PopState(StateManager pManager);

void StateManager_BindShader(StateManager pManager, GLShader pShader);
void StateManager_BindBufferVAO(StateManager pManager, GLBuffer pBuffer);
void StateManager_BindTerrainBufferVAO(StateManager pManager, TerrainGLBuffer pBuffer);
void StateManager_SetViewport(StateManager pManager, int x, int y, int width, int height);
void StateManager_SetScissorRect(StateManager pManager, int x, int y, int width, int height);
void StateManager_SetBlendFunc(StateManager pManager, GLenum src, GLenum dst);
void StateManager_SetCapability(StateManager pManager, EEngineCap cap, bool bEnable);
void StateManager_SetDepthFunc(StateManager pManager, GLenum eDepthFunc);
void StateManager_SetFrontFace(StateManager pManager, GLenum eFrontFace);
void StateManager_SetCullFace(StateManager pManager, GLenum eCullFace);

void StateManager_ApplyState(StateManager pManager, StateSnapshot pStateSnap);
void StateManager_ApplyCapabilities(StateSnapshot pActiveState, StateSnapshot pNewState);
void StateManager_ApplyRasterizer(StateSnapshot pActiveState, StateSnapshot pNewState);

void StateManager_ApplyDepthState(StateSnapshot pActive, StateSnapshot pNewStateSnap);
void StateManager_ApplyBlendState(StateSnapshot pActive, StateSnapshot pNewStateSnap);

void StateManager_ApplyResources(StateSnapshot pActiveState, StateSnapshot pNewState);

StateManager GetStateManager();
static StateManager psStateManager;

#endif // __STATE_MANAGER_H__