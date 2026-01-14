#include "Engine.h"

#pragma comment(lib, "glfw3.lib")
// #pragma comment(lib, "Debug\\assimp-vc143-mtd.lib")

int main(int argc, char* argv[])
{
	Engine engine = (Engine)tracked_calloc(1, sizeof(SEngine));

	if (!InitializeEngine(engine))
	{
		return EXIT_FAILURE;
	}

	while (engine->isRunning && glfwWindowShouldClose(GetGLWindow(engine->window)) == false)
	{
		UpdateEngine(engine);
	}

	DestroyEngine(engine);

	tracked_free(engine);

	printf("Leaks: %zu\n", allocation_count);
	return (EXIT_SUCCESS);
}