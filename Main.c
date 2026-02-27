#include "Stdafx.h"

int main(int argc, char* argv[])
{
	MemoryManager memoryManager;
	if (!MemoryManager_Initialize(&memoryManager))
	{
		syserr("Failed to Initialize Memory Manager");
		return (false);
	}

	Engine engine = engine_new(SEngine, MEM_TAG_ENGINE);

	if (!Engine_Initialize(engine))
	{
		return EXIT_FAILURE;
	}

	while (engine->isRunning && glfwWindowShouldClose(Window_GetGLWindow(engine->window)) == false)
	{
		Engine_Update(engine);
	}

	Engine_Destroy(engine);
	engine_delete(engine);

	MemoryManager_DumpLeaks();

	MemoryManager_Destroy(&memoryManager);
	return (EXIT_SUCCESS);
}