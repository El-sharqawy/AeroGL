#include "Stdafx.h"

// This block forces High Performance GPUs on both AMD and NVIDIA systems
#if defined(__cplusplus)
extern "C" {
#endif
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
#if defined(__cplusplus)
}
#endif

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