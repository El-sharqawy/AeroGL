#ifndef __STATE_MANAGER_H__
#define __STATE_MANAGER_H__

#define MAX_STACKS_ALLOWED 32

#define MAX_TEXTURE_UNITS 32
#define MAX_CAPABILITIES 16

#define MAX_VIEWPORTS 4
#define MAX_SCISSORS 4

#include <glad/glad.h>
#include "../Buffers/Buffer.h"
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
	GLBuffer pCurrentBuffer;
	GLShader pCurrentShader;

	// UI Specifics
	GLint scissorBox[MAX_SCISSORS];			// {x, y, width, height}
	GLint viewport[MAX_VIEWPORTS];			// {x, y, width, height}
	GLenum blendSrc, blendDst;				// How transparency is calculated

	GLenum depthFunc;						// GL_LESS | GL_LEQUAL | etc ..
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
	int top;										// Index of the current active state
} SStateManager;

typedef struct SStateManager* StateManager;

bool InitializeStateManager(StateManager* ppStateManager);
void PushState(StateManager pManager);
void PopState(StateManager pManager);

void BindShader(StateManager pManager, GLShader pShader);
void BindBufferVAO(StateManager pManager, GLBuffer pBuffer);
void SetViewport(StateManager pManager, int x, int y, int width, int height);
void SetScissorRect(StateManager pManager, int x, int y, int width, int height);
void SetBlendFunc(StateManager pManager, GLenum src, GLenum dst);
void SetCapability(StateManager pManager, EEngineCap cap, bool bEnable);

void ApplyState(StateManager pManager, StateSnapshot pStateSnap);

StateSnapshot GetCurrentState(StateManager pManager);
int GetStateDepth(StateManager pManager);
StateManager GetStateManager();

void DestroyStateManager(StateManager* ppManager);

#endif // __STATE_MANAGER_H__