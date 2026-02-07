#include "Engine.h"
#include "Resources/MemoryManager.h"
#include "Lib/UnorderedMap.h"
#include "PipeLine/Texture.h"

#pragma comment(lib, "glfw3.lib")
// #pragma comment(lib, "Debug\\assimp-vc143-mtd.lib")

int main(int argc, char* argv[])
{
	MemoryManager memoryManager;
	if (!MemoryManager_Initialize(&memoryManager))
	{
		syserr("Failed to Initialize Memory Manager");
		return (false);
	}

	Engine engine = engine_new(SEngine, MEM_TAG_ENGINE);
	
	if (!InitializeEngine(engine))
	{
		return EXIT_FAILURE;
	}

	while (engine->isRunning && glfwWindowShouldClose(Window_GetGLWindow(engine->window)) == false)
	{
		UpdateEngine(engine);
	}

	DestroyEngine(engine);
	engine_delete(engine);

	MemoryManager_DumpLeaks();

	MemoryManager_Destroy(&memoryManager);
	return (EXIT_SUCCESS);
}